/**********************************************************************************/
/* This file is part of spla project                                              */
/* https://github.com/SparseLinearAlgebra/spla                                    */
/**********************************************************************************/
/* MIT License                                                                    */
/*                                                                                */
/* Copyright (c) 2023 SparseLinearAlgebra                                         */
/*                                                                                */
/* Permission is hereby granted, free of charge, to any person obtaining a copy   */
/* of this software and associated documentation files (the "Software"), to deal  */
/* in the Software without restriction, including without limitation the rights   */
/* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      */
/* copies of the Software, and to permit persons to whom the Software is          */
/* furnished to do so, subject to the following conditions:                       */
/*                                                                                */
/* The above copyright notice and this permission notice shall be included in all */
/* copies or substantial portions of the Software.                                */
/*                                                                                */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  */
/* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  */
/* SOFTWARE.                                                                      */
/**********************************************************************************/

#include "cl_accelerator.hpp"

#include <opencl/cl_alloc_general.hpp>
#include <opencl/cl_alloc_linear.hpp>
#include <opencl/cl_configure.hpp>
#include <opencl/cl_counter.hpp>
#include <opencl/cl_program_cache.hpp>

#include <sstream>

namespace spla {

    CLAccelerator::CLAccelerator()  = default;
    CLAccelerator::~CLAccelerator() = default;

    Status CLAccelerator::init_with_configure(int argc, char** argv) {
        ConfigStatus status = configure(argc, argv);
        if (status == ConfigStatus::HelpRequested ||
            status == ConfigStatus::VersionRequested) {
            return Status::Ok;
        }
        if (status != ConfigStatus::Ok) return Status::Error;

        auto* acc = get_acc_cl();
        if (!acc) return Status::Error;
        return acc->init();
    }
    

    Status CLAccelerator::init() {
        m_description = "no platform or device";

        Config cfg;
        cfg = get_config();

        int         platform_index = cfg.platform.value();
        int         device_index   = cfg.device.value();
        int         queues_count   = cfg.queues.value();
        bool        profiling      = cfg.profiling.value();
        std::string allocator_type = cfg.allocator.value();
        size_t lin_allocator_size = 0;
        if (allocator_type == "linear") {
            lin_allocator_size = cfg.allocator_size.value_or(0);
        }
        int verbosity = cfg.verbosity.value();

        std::cout << "Configuration parametrs:" << std::endl;
        std::cout << "OpenCL platform index: " << platform_index << std::endl;
        std::cout << "OpenCL device index: " << device_index << std::endl;
        std::cout << "Queues number: " << queues_count << std::endl;
        std::cout << "Profiling: " << profiling << std::endl;
        std::cout << "Allocator: " << allocator_type << std::endl;
        if (allocator_type == "linear") std::cout << "Linear allocator size: " << cfg.allocator_size.value() << std::endl;
        std::cout << "Verbosity: " << verbosity << std::endl;

        if (set_platform(platform_index) != Status::Ok)
            return Status::PlatformNotFound;

        if (set_device(device_index) != Status::Ok)
            return Status::DeviceNotFound;

        if (set_profiling(profiling) != Status::Ok)
            return Status::Error;

        if (set_queues_count(queues_count) != Status::Ok)
            return Status::Error;

        if (allocator_type == "linear") {
            if (set_linear_allocator(cfg.allocator_size.value()) != Status::Ok)
                return Status::Error;
        } else {
            if (set_general_allocator() != Status::Ok)
                return Status::Error;
        }

        m_cache = std::make_unique<CLProgramCache>();

        // Output handy info
        LOG_MSG(Status::Ok, "Initialize accelerator: " << get_description());

        return Status::Ok;
    }


    Status CLAccelerator::set_platform(int index) {
        std::vector<cl::Platform> available_platforms;
        cl::Platform::get(&available_platforms);

        if (available_platforms.empty()) {
            LOG_MSG(Status::PlatformNotFound, "no platform to select for OpenCL acceleration");
            return Status::PlatformNotFound;
        }

        if (index < 0) {
            LOG_MSG(Status::InvalidArgument, "platform index must be >= 0 (got " << index << ")");
            return Status::InvalidArgument;
        }

        if (available_platforms.size() <= static_cast<size_t>(index)) {
            LOG_MSG(Status::InvalidArgument, "platform index out of range (got " << index << ", max " << available_platforms.size() - 1 << ")");
            return Status::InvalidArgument;
        }

        m_counter_pool.reset();
        m_alloc_general.reset();
        m_alloc_linear.reset();
        m_alloc_tmp = nullptr;
        m_device    = cl::Device();
        m_profiling_enabled = false;

        m_platform = available_platforms[index];
        LOG_MSG(Status::Ok, "select OpenCL platform " << m_platform.getInfo<CL_PLATFORM_NAME>());

        return Status::Ok;
    }


    Status CLAccelerator::set_device(int index) {
        std::vector<cl::Device> available_devices;
        m_platform.getDevices(CL_DEVICE_TYPE_GPU, &available_devices);

        if (available_devices.empty()) {
            LOG_MSG(Status::DeviceNotFound, "no device to select for OpenCL acceleration");
            return Status::DeviceNotFound;
        }

        if (index < 0) {
            LOG_MSG(Status::InvalidArgument, "device index must be >= 0 (got " << index << ")");
            return Status::InvalidArgument;
        }

        if (available_devices.size() <= static_cast<size_t>(index)) {
            LOG_MSG(Status::InvalidArgument, "platform index out of range (got " << index << ", max " << available_devices.size() - 1 << ")");
            return Status::InvalidArgument;
        }

        m_device = available_devices[index];
        LOG_MSG(Status::Ok, "select OpenCL device " << m_device.getInfo<CL_DEVICE_NAME>());

        m_vendor_code.clear();
        m_vendor_name   = m_device.getInfo<CL_DEVICE_VENDOR>();
        m_vendor_id     = m_device.getInfo<CL_DEVICE_VENDOR_ID>();
        m_max_cu        = m_device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
        m_max_wgs       = m_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
        m_max_local_mem = m_device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
        m_addr_align    = m_device.getInfo<CL_DEVICE_MEM_BASE_ADDR_ALIGN>() / 8;// from bits to bytes

        m_is_nvidia = false;
        m_is_amd    = false;
        m_is_intel  = false;
        m_is_img    = false;

        if (m_vendor_name.find("Intel") != std::string::npos ||
            m_vendor_name.find("intel") != std::string::npos ||
            m_vendor_name.find("INTEL") != std::string::npos ||
            m_vendor_id == 32902) {
            m_vendor_code = VENDOR_CODE_INTEL;
            m_default_wgs = 64;
            m_wave_size   = 8;
            m_is_intel    = true;
        }
        if (m_vendor_name.find("Nvidia") != std::string::npos ||
            m_vendor_name.find("nvidia") != std::string::npos ||
            m_vendor_name.find("NVIDIA") != std::string::npos ||
            m_vendor_id == 4318) {
            m_vendor_code = VENDOR_CODE_NVIDIA;
            m_default_wgs = 64;
            m_wave_size   = 32;
            m_is_nvidia   = true;
        }
        if (m_vendor_name.find("Amd") != std::string::npos ||
            m_vendor_name.find("amd") != std::string::npos ||
            m_vendor_name.find("AMD") != std::string::npos ||
            m_vendor_name.find("Advanced Micro Devices") != std::string::npos ||
            m_vendor_name.find("advanced micro devices") != std::string::npos ||
            m_vendor_name.find("ADVANCED MICRO DEVICES") != std::string::npos) {
            m_vendor_code = VENDOR_CODE_AMD;
            m_default_wgs = 64;
            m_wave_size   = 64;
            m_is_amd      = true;

            // Likely, it is an integrated amd device
            if (m_max_wgs <= 256 || m_max_cu == 1) m_wave_size = 16;
        }
        if (m_vendor_name.find("Imagination Technologies") != std::string::npos ||
            m_vendor_name.find("IMG") != std::string::npos ||
            m_vendor_name.find("img") != std::string::npos ||
            m_vendor_id == 0x1010) {
            m_vendor_code = VENDOR_CODE_IMG;
            m_default_wgs = 32;
            m_wave_size   = 32;
            m_is_img      = true;
        }

        if (m_vendor_code.empty()) {
            LOG_MSG(Status::Error, "failed to match one of the pre-defined vendors");
            m_default_wgs = 64;
            m_wave_size   = 8;
        }

        std::stringstream desc;
        desc << "OpenCL Acc " << m_platform.getInfo<CL_PLATFORM_NAME>()
             << " device: " << m_device.getInfo<CL_DEVICE_NAME>()
             << " vendor:" << m_vendor_code
             << " mcu:" << m_max_cu
             << " wave:" << m_wave_size
             << " mwgs:" << m_max_wgs;

        m_description = desc.str();

        LOG_MSG(Status::Ok, m_description);
        return Status::Ok;
    }

    Status CLAccelerator::set_profiling(bool enabled) {
        m_profiling_enabled = enabled;
        LOG_MSG(Status::Ok, "set profiling " << (enabled ? "enabled" : "disabled"));
        return Status::Ok;
    }


    Status CLAccelerator::set_queues_count(int count) {
        if (count <= 0) {
            LOG_MSG(Status::InvalidArgument, "queues count must be > 0 (got " << count << ")");
            return Status::InvalidArgument;
        }

        m_context = cl::Context(m_device);
        m_queues.clear();
        m_queues.reserve(count);

        for (int i = 0; i < count; i++) {
            cl_command_queue_properties properties = 0;
            if (m_profiling_enabled) {
                properties |= CL_QUEUE_PROFILING_ENABLE;
            }
            cl::CommandQueue queue(m_context, properties);
            m_queues.emplace_back(std::move(queue));
        }

        m_counter_pool  = std::make_unique<CLCounterPool>();
        m_alloc_general = std::make_unique<CLAllocGeneral>();
        m_alloc_tmp     = m_alloc_general.get();

        LOG_MSG(Status::Ok, "configure " << count << " queues for computations"
                                         << " (profiling: " << (m_profiling_enabled ? "ON" : "OFF") << ")");
        return Status::Ok;
    }


    Status CLAccelerator::set_linear_allocator(size_t size) {
        if (size == 0) {
            LOG_MSG(Status::InvalidArgument, "allocator_size must be > 0 for linear allocator (got " << size << ")");
            return Status::InvalidArgument;
        }
        m_alloc_linear = std::make_unique<CLAllocLinear>(size, m_addr_align);
        m_alloc_tmp    = m_alloc_linear.get();
        LOG_MSG(Status::Ok, "set linear allocator (size: " << size << " bytes)");
        return Status::Ok;
    }


    Status CLAccelerator::set_general_allocator() {
        if (!m_alloc_general) {
            LOG_MSG(Status::Error, "general allocator not initialized");
            return Status::Error;
        }
        m_alloc_tmp = m_alloc_general.get();
        LOG_MSG(Status::Ok, "set general allocator");
        return Status::Ok;
    }

    const std::string& CLAccelerator::get_name() {
        return m_name;
    }
    const std::string& CLAccelerator::get_description() {
        return m_description;
    }
    const std::string& CLAccelerator::get_suffix() {
        return m_suffix;
    }

}// namespace spla