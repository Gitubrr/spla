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
        if (src.backend.has_value()) backend = src.backend;
        if (src.platform_index.has_value()) platform_index = src.platform_index;
        if (src.device_index.has_value()) device_index = src.device_index;
        if (src.if_gpu_unavailable.has_value()) if_gpu_unavailable = src.if_gpu_unavailable;
        if (src.queues_count.has_value()) queues_count = src.queues_count;
        if (src.profiling.has_value()) profiling = src.profiling;
        if (src.allocator_type.has_value()) allocator_type = src.allocator_type;
        if (src.linear_alloc_size.has_value()) linear_alloc_size = src.linear_alloc_size;
        if (src.verbosity.has_value()) verbosity = src.verbosity;
        if (src.extends.has_value()) extends = src.extends;
    }


    void Config::merge(const Config& src) {
        if (src.backend.has_value()) backend = src.backend;
        if (src.platform_index.has_value()) platform_index = src.platform_index;
        if (src.device_index.has_value()) device_index = src.device_index;
        if (src.if_gpu_unavailable.has_value()) if_gpu_unavailable = src.if_gpu_unavailable;
        if (src.queues_count.has_value()) queues_count = src.queues_count;
        if (src.profiling.has_value()) profiling = src.profiling;
        if (src.allocator_type.has_value()) allocator_type = src.allocator_type;
        if (src.linear_alloc_size.has_value()) linear_alloc_size = src.linear_alloc_size;
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

        app.set_help_flag("-sh,--spla-help", "Show help and exit");
        app.add_flag("-sv,--spla-version", cfg.version, "Show version and exit");

        app.add_option("-sb,--spla-backend", cfg.backend,
                       "Device type to use:\n"
                       "  gpu       - only GPU\n"
                       "  cpu       - only CPU\n"
                       "  any       - any available device\n"
                       "  by_index  - select by platform_index and device_index\n"
                       "Config key: backend")
                ->envname("SPLA_BACKEND");

        app.add_option("-sp,--spla-platform-index", cfg.platform_index,
                       "OpenCL platform index\n"
                       "Used only with backend=by_index.\n"
                       "Config key: platform_index")
                ->envname("SPLA_PLATFORM_INDEX");

        app.add_option("-sd,--spla-device-index", cfg.device_index,
                       "OpenCL device index\n"
                       "Used only with backend=by_index.\n"
                       "Config key: device_index")
                ->envname("SPLA_DEVICE_INDEX");

        app.add_option("-sf,--spla-if-gpu-unavailable", cfg.if_gpu_unavailable,
                       "Behavior when GPU is unavailable:\n"
                       "  use_cpu - switch to CPU\n"
                       "  abort   - return error\n"
                       "Config key: if_gpu_unavailable")
                ->envname("SPLA_IF_GPU_UNAVAILABLE");

        app.add_option("-sq,--spla-queues-count", cfg.queues_count,
                       "Number of command queues\n"
                       "Config key: queues_count")
                ->envname("SPLA_QUEUES_COUNT");

        app.add_flag("-pr,--spla-profiling", cfg.profiling,
                     "Enable profiling of command queues\n"
                     "Config key: profiling")
                ->envname("SPLA_PROFILING");

        app.add_option("-sa,--spla-allocator-type", cfg.allocator_type,
                       "Allocator type:\n"
                       "  general\n"
                       "  linear \n"
                       "Config key: allocator_type")
                ->envname("SPLA_ALLOCATOR_TYPE");

        app.add_option("-as,--spla-linear-alloc-size", cfg.linear_alloc_size,
                       "Linear allocator size in bytes\n"
                       "Required for allocator_type=linear. Ignored for general.\n"
                       "Config key: linear_alloc_size")
                ->envname("SPLA_LINEAR_ALLOC_SIZE");

        app.add_option("-sV,--spla-verbosity", cfg.verbosity,
                       "Verbosity level:\n"
                       "  0 - no output\n"
                       "  1 - errors only\n"
                       "  2 - errors and warnings\n"
                       "  3 - all messages\n"
                       "Config key: verbosity")
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

            if (data.contains("backend")) cfg.backend = data["backend"].get<std::string>();

            if (data.contains("platform_index")) cfg.platform_index = data["platform_index"].get<int>();
            if (data.contains("device_index")) cfg.device_index = data["device_index"].get<int>();

            if (data.contains("if_gpu_unavailable")) cfg.if_gpu_unavailable = data["if_gpu_unavailable"].get<std::string>();

            if (data.contains("queues_count")) cfg.queues_count = data["queues_count"].get<int>();
            if (data.contains("profiling")) cfg.profiling = data["profiling"].get<bool>();
            if (data.contains("allocator_type")) cfg.allocator_type = data["allocator_type"].get<std::string>();
            if (data.contains("linear_alloc_size")) cfg.linear_alloc_size = data["linear_alloc_size"].get<size_t>();

            if (data.contains("verbosity")) cfg.verbosity = data["verbosity"].get<int>();

            if (data.contains("profile")) cfg.profile = data["profile"].get<std::string>();

            if (data.contains("profiles")) {
                std::map<std::string, Profile> profiles;
                for (auto& [name, p] : data["profiles"].items()) {
                    Profile prof;

                    if (p.contains("backend")) prof.backend = p["backend"].get<std::string>();

                    if (p.contains("platform_index")) prof.platform_index = p["platform_index"].get<int>();
                    if (p.contains("device_index")) prof.device_index = p["device_index"].get<int>();

                    if (p.contains("if_gpu_unavailable")) prof.if_gpu_unavailable = p["if_gpu_unavailable"].get<std::string>();

                    if (p.contains("queues_count")) prof.queues_count = p["queues_count"].get<int>();
                    if (p.contains("profiling")) prof.profiling = p["profiling"].get<bool>();
                    if (p.contains("allocator_type")) prof.allocator_type = p["allocator_type"].get<std::string>();
                    if (p.contains("linear_alloc_size")) prof.linear_alloc_size = p["linear_alloc_size"].get<size_t>();

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


    ConfigStatus validation(const Config& cfg) {
        if (!cfg.backend.has_value()) {
            std::cerr << "[spla:validation] ERROR: required parameter missing: backend" << std::endl;
            return ConfigStatus::MissedParameters;
        }

        const std::string& backend = *cfg.backend;
        if (backend != "gpu" && backend != "cpu" &&
            backend != "any" && backend != "by_index") {
            std::cerr << "[spla:validation] ERROR: backend must be "
                      << "'gpu', 'cpu', 'any' or 'by_index' (got '" << backend << "')" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }

        if (backend == "by_index") {
            if (!cfg.platform_index.has_value()) {
                std::cerr << "[spla:validation] ERROR: platform_index is required "
                          << "when backend='by_index'" << std::endl;
                return ConfigStatus::MissedParameters;
            }
            if (!cfg.device_index.has_value()) {
                std::cerr << "[spla:validation] ERROR: device_index is required "
                          << "when backend='by_index'" << std::endl;
                return ConfigStatus::MissedParameters;
            }

            if (cfg.platform_index < 0) {
                std::cerr << "[spla:opencl] ERROR: platform must be >= 0 (got "
                          << *cfg.platform_index << ")" << std::endl;
                return ConfigStatus::InvalidConfigParams;
            }
            if (cfg.device_index < 0) {
                std::cerr << "[spla:opencl] ERROR: device must be >= 0 (got "
                          << *cfg.device_index << ")" << std::endl;
                return ConfigStatus::InvalidConfigParams;
            }
        }

        if (backend != "cpu") {
            if (!cfg.if_gpu_unavailable.has_value()) {
                std::cerr << "[spla:validation] ERROR: if_gpu_unavailable is required "
                          << "when backend= 'any', 'gpu', 'by_index'" << std::endl;
                return ConfigStatus::MissedParameters;
            }
            const std::string& behavior = *cfg.if_gpu_unavailable;
            if (behavior != "use_cpu" && behavior != "abort") {
                std::cerr << "[spla:validation] ERROR: if_gpu_unavailable must be "
                          << "'use_cpu' or 'abort' (got '" << behavior << "')" << std::endl;
                return ConfigStatus::InvalidConfigParams;
            }
        }
        if (backend == "cpu" && cfg.if_gpu_unavailable.has_value()) {
            std::cerr << "[spla:validation] WARNING: if_gpu_unavailable "
                      << "ignored for backend='" << backend << "'" << std::endl;
        }


        if (backend != "by_index") {
            if (cfg.platform_index.has_value() || cfg.device_index.has_value()) {
                std::cerr << "[spla:validation] WARNING: platform_index/device_index "
                          << "ignored for backend='" << backend << "'" << std::endl;
            }
        }


        if (!cfg.queues_count.has_value()) {
            std::cerr << "[spla:validation] ERROR: required parameter missing: queues_count" << std::endl;
            return ConfigStatus::MissedParameters;
        }
        if (*cfg.queues_count <= 0) {
            std::cerr << "[spla:validation] ERROR: queues_count must be > 0 (got "
                      << *cfg.queues_count << ")" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }


        if (!cfg.profiling.has_value()) {
            std::cerr << "[spla:validation] ERROR: required parameter missing: profiling" << std::endl;
            return ConfigStatus::MissedParameters;
        }


        if (!cfg.allocator_type.has_value()) {
            std::cerr << "[spla:validation] ERROR: required parameter missing: allocator_type" << std::endl;
            return ConfigStatus::MissedParameters;
        }
        if (*cfg.allocator_type != "general" && *cfg.allocator_type != "linear") {
            std::cerr << "[spla:validation] ERROR: allocator_type must be "
                      << "'general' or 'linear' (got '" << *cfg.allocator_type << "')" << std::endl;
            return ConfigStatus::InvalidConfigParams;
        }


        if (*cfg.allocator_type == "linear") {
            if (!cfg.linear_alloc_size.has_value()) {
                std::cerr << "[spla:validation] ERROR: linear_alloc_size is required "
                          << "when allocator_type='linear'" << std::endl;
                return ConfigStatus::MissedParameters;
            }
            if (*cfg.linear_alloc_size <= 0) {
                std::cerr << "[spla:validation] ERROR: linear_alloc_size must be > 0 (got "
                          << *cfg.linear_alloc_size << ")" << std::endl;
                return ConfigStatus::InvalidConfigParams;
            }
        }


        if (!cfg.verbosity.has_value()) {
            std::cerr << "[spla:validation] ERROR: required parameter missing: verbosity" << std::endl;
            return ConfigStatus::MissedParameters;
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

        if (resolved.backend) cfg.backend = resolved.backend;
        if (resolved.if_gpu_unavailable) cfg.if_gpu_unavailable = resolved.if_gpu_unavailable;
        if (resolved.platform_index) cfg.platform_index = resolved.platform_index;
        if (resolved.device_index) cfg.device_index = resolved.device_index;
        if (resolved.queues_count) cfg.queues_count = resolved.queues_count;
        if (resolved.profiling) cfg.profiling = resolved.profiling;
        if (resolved.allocator_type) cfg.allocator_type = resolved.allocator_type;
        if (resolved.linear_alloc_size) cfg.linear_alloc_size = resolved.linear_alloc_size;
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
