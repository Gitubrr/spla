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

#include "cl_configure.hpp"
#include "CL/opencl.hpp"
#include "cl_accelerator.hpp"

#include <filesystem>
#include <fstream>

namespace spla {

    Config config_default;
    Config config_system;
    Config config_user;
    Config config_cli_and_env;
    Config config_final;


    void Profile::merge(const Profile& src) {
        if (src.platform.has_value()) platform = src.platform;
        if (src.device.has_value()) device = src.device;
        if (src.queues.has_value()) queues = src.queues;
        if (src.profiling.has_value()) profiling = src.profiling;
        if (src.allocator.has_value()) allocator = src.allocator;
        if (src.allocator_size.has_value()) allocator_size = src.allocator_size;
        if (src.verbosity.has_value()) verbosity = src.verbosity;
        if (src.extends.has_value()) extends = src.extends;
    }


    void Config::merge(const Config& src) {
        if (src.platform.has_value()) platform = src.platform;
        if (src.device.has_value()) device = src.device;
        if (src.queues.has_value()) queues = src.queues;
        if (src.profiling.has_value()) profiling = src.profiling;
        if (src.allocator.has_value()) allocator = src.allocator;
        if (src.allocator_size.has_value()) allocator_size = src.allocator_size;
        if (src.verbosity.has_value()) verbosity = src.verbosity;

        if (src.profile.has_value()) profile = src.profile;

        if (src.profiles.has_value()) {
            if (!profiles.has_value()) {
                profiles = src.profiles;
            } else {
                for (const auto& [name, src_prof] : *src.profiles) {
                    auto& dst_prof = (*profiles)[name];
                    dst_prof.merge(src_prof);
                }
            }
        }
    }

    void Config::reset() {
        *this = Config{};
    }

    std::string get_spla_version() {
        return "SPLA version: 0.0.0";
    }


    std::string get_default_config_path() {
#ifdef _WIN32
        if (const char* pd = std::getenv("PROGRAMDATA")) {
            return std::string(pd) + "\\spla\\spla_conf.json";
        }
        return "C:\\ProgramData\\spla\\spla_conf.json";

#elif defined(__APPLE__)
        return "/usr/local/share/spla/spla_conf.json";

#elif defined(__linux__)
        return "/usr/share/spla/spla_conf.json";

#else
    #error "spla supports only Windows, macOS and Linux"
#endif
    }


    std::string get_default_system_config_path() {
#ifdef _WIN32
        const char* program_data = std::getenv("PROGRAMDATA");
        if (program_data) {
            return std::string(program_data) + "\\spla\\spla_conf.json";
        }
        return "C:\\ProgramData\\spla\\spla_conf.json";

#elif defined(__APPLE__)
        return "/Library/Application Support/spla/spla_conf.json";

#elif defined(__linux__)
        return "/etc/spla/spla_conf.json";

#else
    #error "spla supports only Windows, macOS and Linux"
#endif
    }


    std::string get_home_directory() {
#ifdef _WIN32
        const char* home = std::getenv("USERPROFILE");
        if (home) return std::string(home);

        const char* drive = std::getenv("HOMEDRIVE");
        const char* path  = std::getenv("HOMEPATH");
        if (drive && path) return std::string(drive) + std::string(path);
        throw std::runtime_error("Cannot determine home directory");

#elif defined(__APPLE__) || defined(__linux__)
        const char* home = std::getenv("HOME");
        if (home) return std::string(home);

        struct passwd* pw = getpwuid(getuid());
        if (pw) return std::string(pw->pw_dir);
        throw std::runtime_error("Cannot determine home directory");

#else
    #error "spla supports only Windows, macOS and Linux"
#endif
    }


    std::string get_default_user_config_path() {
        std::string home = get_home_directory();

#ifdef _WIN32
        const char* app_data = std::getenv("APPDATA");
        if (app_data) return std::string(app_data) + "\\spla\\spla_conf.json";
        return home + "\\AppData\\Roaming\\spla\\spla_conf.json";

#elif defined(__APPLE__)
        return home + "/Library/Application Support/spla/spla_conf.json";

#elif defined(__linux__) || defined(__unix__)
        return home + "/.config/spla/spla_conf.json";

#else
    #error "spla supports only Windows, macOS and Linux"
#endif
    }


    ConfigStatus parse_cli_and_env(int argc, char** argv, Config& cfg) {
        CLI::App app{"SPLA configuration"};

        app.add_flag("-sh,--spla-help", cfg.help, "Show help and exit");
        app.add_flag("-sv,--spla-version", cfg.version, "Show version and exit");

        app.add_option("-sp,--spla-platform", cfg.platform,
                       "OpenCL platform index\n"
                       "Config key: platform")
                ->envname("SPLA_OPENCL_PLATFORM");

        app.add_option("-sd,--spla-device", cfg.device,
                       "OpenCL device index\n"
                       "Config key: device")
                ->envname("SPLA_OPENCL_DEVICE");

        app.add_option("-sq,--spla-queues", cfg.queues,
                       "Number of command queues\n"
                       "Config key: queues")
                ->envname("SPLA_QUEUES");

        app.add_flag("-pr,--spla-profiling", cfg.profiling,
                     "Enable profiling of command queues\n"
                     "Config key: profiling\n")
                ->envname("SPLA_PROFILING");

        app.add_option("-sa,--spla-allocator", cfg.allocator,
                       "Allocator type: linear or general\n"
                       "Config key: allocator")
                ->envname("SPLA_ALLOCATOR");

        app.add_option("-as,--spla-allocator-size", cfg.allocator_size,
                       "Linear allocator size in bytes\n"
                       "Required for 'linear' allocator. Ignored for 'general'.\n"
                       "Config key: allocator_size")
                ->envname("SPLA_ALLOCATOR_SIZE");

        app.add_option("-sV,--spla-verbosity", cfg.verbosity,
                       "Verbosity level:\n"
                       "  0: No output\n"
                       "  1: Errors only\n"
                       "  2: Errors + warnings\n"
                       "  3: All messages (info, warnings, errors)")
                ->envname("SPLA_VERBOSITY");

        app.add_option("-sP,--spla-profile", cfg.profile,
                       "Configuration profile name\n"
                       "Overrides base settings with the named profile.\n"
                       "Config key: profile")
                ->envname("SPLA_PROFILE");

        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            std::cerr << "[spla:cli_env] ERROR: failed to parse: " << e.what() << std::endl;
            return ConfigStatus::CliOrEnvParseError;
        }

        if (cfg.help) {
            std::cout << app.help() << std::endl;
            return ConfigStatus::HelpRequested;
        }

        if (cfg.version) {
            std::cout << get_spla_version() << std::endl;
            return ConfigStatus::VersionRequested;
        }

        return ConfigStatus::Ok;
    }


    ConfigStatus parse_file(const std::string& path, Config& cfg) {

        if (!std::filesystem::exists(path)) {
            std::cerr << "[spla:config] WARNING: file not found: " << path << std::endl;
            return ConfigStatus::Ok;
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[spla:config] WARNING: cannot open: " << path << std::endl;
            return ConfigStatus::OpenFileError;
        }

        try {
            nlohmann::json data = nlohmann::json::parse(file);

            if (data.contains("platform")) cfg.platform = data["platform"].get<int>();
            if (data.contains("device")) cfg.device = data["device"].get<int>();
            if (data.contains("queues")) cfg.queues = data["queues"].get<int>();
            if (data.contains("profiling")) cfg.profiling = data["profiling"].get<bool>();
            if (data.contains("allocator")) cfg.allocator = data["allocator"].get<std::string>();
            if (data.contains("allocator_size")) cfg.allocator_size = data["allocator_size"].get<size_t>();
            if (data.contains("verbosity")) cfg.verbosity = data["verbosity"].get<int>();
            if (data.contains("profile")) cfg.profile = data["profile"].get<std::string>();

            if (data.contains("profiles")) {
                std::map<std::string, Profile> profiles;
                for (auto& [name, p] : data["profiles"].items()) {
                    Profile prof;
                    if (p.contains("platform")) prof.platform = p["platform"].get<int>();
                    if (p.contains("device")) prof.device = p["device"].get<int>();
                    if (p.contains("queues")) prof.queues = p["queues"].get<int>();
                    if (p.contains("profiling")) prof.profiling = p["profiling"].get<bool>();
                    if (p.contains("allocator")) prof.allocator = p["allocator"].get<std::string>();
                    if (p.contains("allocator_size")) prof.allocator_size = p["allocator_size"].get<size_t>();
                    if (p.contains("verbosity")) prof.verbosity = p["verbosity"].get<int>();
                    if (p.contains("extends")) prof.extends = p["extends"].get<std::vector<std::string>>();
                    profiles[name] = prof;
                }
                cfg.profiles = profiles;
            }

            return ConfigStatus::Ok;

        } catch (const nlohmann::json::exception& e) {
            std::cerr << "[spla:config] ERROR: failed to parse '"
                      << path << "': " << e.what() << std::endl;
            return ConfigStatus::ParseConfError;
        }
    }


    ConfigStatus check_platform_and_device(int platform_index, int device_index) {

        if (platform_index < 0) {
            std::cerr << "[spla:opencl] ERROR: platform must be >= 0 (got "
                      << platform_index << ")" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }

        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        if (platforms.empty()) {
            std::cerr << "[spla:opencl] ERROR: no platform available" << std::endl;
            return ConfigStatus::PlatformNotFound;
        }

        if (static_cast<size_t>(platform_index) >= platforms.size()) {
            std::cerr << "[spla:opencl] ERROR: platform index out of range (got "
                      << platform_index << ", max " << platforms.size() - 1 << ")"
                      << std::endl;
            return ConfigStatus::PlatformNotFound;
        }

        if (device_index < 0) {
            std::cerr << "[spla:opencl] ERROR: device must be >= 0 (got "
                      << device_index << ")" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }

        std::vector<cl::Device> devices;
        platforms[platform_index].getDevices(CL_DEVICE_TYPE_ALL, &devices);

        if (devices.empty()) {
            std::cerr << "[spla:opencl] ERROR: no device available on platform "
                      << platform_index << std::endl;
            return ConfigStatus::DeviceNotFound;
        }

        if (static_cast<size_t>(device_index) >= devices.size()) {
            std::cerr << "[spla:opencl] ERROR: device index out of range (got "
                      << device_index << ", max " << devices.size() - 1 << ")"
                      << std::endl;
            return ConfigStatus::DeviceNotFound;
        }

        return ConfigStatus::Ok;
    }


    ConfigStatus validation(const Config& cfg) {

        if (!cfg.platform.has_value()) {
            std::cerr << "[spla:validation] ERROR: required: platform" << std::endl;
            return ConfigStatus::MissedParameters;
        }
        if (!cfg.device.has_value()) {
            std::cerr << "[spla:validation] ERROR: required: device" << std::endl;
            return ConfigStatus::MissedParameters;
        }
        if (!cfg.queues.has_value()) {
            std::cerr << "[spla:validation] ERROR: required: queues" << std::endl;
            return ConfigStatus::MissedParameters;
        }
        if (!cfg.profiling.has_value()) {
            std::cerr << "[spla:validation] ERROR: required: profiling" << std::endl;
            return ConfigStatus::MissedParameters;
        }
        if (!cfg.allocator.has_value()) {
            std::cerr << "[spla:validation] ERROR: required: allocator" << std::endl;
            return ConfigStatus::MissedParameters;
        }
        if (!cfg.verbosity.has_value()) {
            std::cerr << "[spla:validation] ERROR: required: verbosity" << std::endl;
            return ConfigStatus::MissedParameters;
        }

        if (cfg.allocator.value() == "linear" && !cfg.allocator_size.has_value()) {
            std::cerr << "[spla:validation] ERROR: allocator_size is required for 'linear' allocator"
                      << std::endl;
            return ConfigStatus::MissedParameters;
        }

        ConfigStatus status;

        status = check_platform_and_device(*cfg.platform, *cfg.device);
        if (status != ConfigStatus::Ok) {
            return status;
        }

        if (*cfg.queues <= 0) {
            std::cerr << "[spla:validation] ERROR: queues must be > 0 (got "
                      << *cfg.queues << ")" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }

        if (*cfg.allocator != "linear" && *cfg.allocator != "general") {
            std::cerr << "[spla:validation] ERROR: allocator must be 'linear' or 'general' (got '"
                      << *cfg.allocator << "')" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }

        if (*cfg.allocator == "linear" && *cfg.allocator_size <= 0) {
            std::cerr << "[spla:validation] ERROR: allocator_size must be > 0 for 'linear' (got "
                      << *cfg.allocator_size << ")" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }

        if (*cfg.verbosity < 0 || *cfg.verbosity > 3) {
            std::cerr << "[spla:validation] ERROR: verbosity must be in [0, 3] (got "
                      << *cfg.verbosity << ")" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }

        return ConfigStatus::Ok;
    }


    Profile apply_extends(const std::map<std::string, Profile>& profiles,
                          const std::string&                    name,
                          std::set<std::string>&                stack) {
        if (stack.count(name)) {
            throw std::runtime_error("Profile cycle detected: " + name);
        }
        stack.insert(name);

        auto it = profiles.find(name);
        if (it == profiles.end()) {
            throw std::runtime_error("Profile not found: " + name);
        }

        const Profile& prof = it->second;
        Profile        result;

        if (prof.extends) {
            for (const auto& parent_name : *prof.extends) {
                Profile parent = apply_extends(profiles, parent_name, stack);
                result.merge(parent);
            }
        }

        result.merge(prof);

        stack.erase(name);
        return result;
    }

    ConfigStatus apply_profile(Config& cfg) {

        if (!cfg.profile.has_value()) return ConfigStatus::Ok;

        std::string profile_name = *cfg.profile;

        if (!cfg.profiles || !cfg.profiles->count(profile_name)) {
            std::cerr << "[spla:profile] ERROR: not found: " << profile_name << std::endl;
            return ConfigStatus::ProfileNotFound;
        }

        std::set<std::string> stack;
        Profile               resolved;
        try {
            resolved = apply_extends(*cfg.profiles, profile_name, stack);
        } catch (const std::runtime_error& e) {
            std::string msg = e.what();
            if (msg.find("cycle") != std::string::npos) {
                std::cerr << "[spla:profile] ERROR: cycle detected: "
                          << profile_name << std::endl;
                return ConfigStatus::ProfileCycle;
            }
            std::cerr << "[spla:profile] ERROR: " << msg << std::endl;
            return ConfigStatus::ProfileNotFound;
        }

        if (resolved.platform) cfg.platform = resolved.platform;
        if (resolved.device) cfg.device = resolved.device;
        if (resolved.queues) cfg.queues = resolved.queues;
        if (resolved.profiling) cfg.profiling = resolved.profiling;
        if (resolved.allocator) cfg.allocator = resolved.allocator;
        if (resolved.allocator_size) cfg.allocator_size = resolved.allocator_size;
        if (resolved.verbosity) cfg.verbosity = resolved.verbosity;

        return ConfigStatus::Ok;
    }


    ConfigStatus configure(int argc, char** argv) {
        config_default.reset();
        config_system.reset();
        config_user.reset();
        config_cli_and_env.reset();
        config_final.reset();

        ConfigStatus status;

        status = parse_cli_and_env(argc, argv, config_cli_and_env);
        if (status != ConfigStatus::Ok) return status;

        status = parse_file(get_default_user_config_path(), config_user);
        if (status == ConfigStatus::ParseConfError) return status;
        if (status == ConfigStatus::OpenFileError) {
            std::cerr << "[spla:configure] WARNING: cannot open user config, skipping" << std::endl;
        }

        status = parse_file(get_default_system_config_path(), config_system);
        if (status == ConfigStatus::ParseConfError) return status;
        if (status == ConfigStatus::OpenFileError) {
            std::cerr << "[spla:configure] WARNING: cannot open system config, skipping" << std::endl;
        }

        status = parse_file(get_default_config_path(), config_default);
        if (status == ConfigStatus::ParseConfError) return status;
        if (status == ConfigStatus::OpenFileError) {
            std::cerr << "[spla:configure] WARNING: cannot open default config, skipping" << std::endl;
        }

        config_final.merge(config_default);
        config_final.merge(config_system);
        config_final.merge(config_user);
        config_final.merge(config_cli_and_env);

        status = apply_profile(config_final);
        if (status != ConfigStatus::Ok) return status;

        status = validation(config_final);
        if (status != ConfigStatus::Ok) return status;

        std::cerr << "[spla:configure]: configuration complete" << std::endl;
        return ConfigStatus::Ok;
    }
}// namespace spla
