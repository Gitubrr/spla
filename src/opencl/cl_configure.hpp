#pragma once

#include <optional>
#include <string>

#include "CLI/CLI.hpp"
#include <nlohmann/json.hpp>

#ifdef _WIN32
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
    #include <pwd.h>
    #include <unistd.h>
#endif

namespace spla {

    struct Config {
        std::optional<bool>        help;
        std::optional<bool>        version;
        std::optional<int>         platform;
        std::optional<int>         device;
        std::optional<int>         queues;
        std::optional<bool>        profiling;
        std::optional<std::string> allocator;
        std::optional<size_t>      allocator_size;
        std::optional<int>         verbosity;

        void merge(const Config& source);
        void reset();
    };

    enum ConfigStatus {
        Ok,
        HelpRequested,
        VersionRequested,

        CliOrEnvParseError,
        ParseConfError,
        OpenFileError,

        MissedParametrs,
        PlatformNotFound,
        DeviceNotFound,
        InvalidConfigParams,
        Error
    };

    extern Config config_default;
    extern Config config_system;
    extern Config config_user;
    extern Config config_cli_and_env;
    extern Config config_final;

    inline Config get_config() { return config_final; }
    std::string   get_spla_version();
    std::string   get_default_config_path();
    std::string   get_default_system_config_path();
    std::string   get_home_directory();
    std::string   get_default_user_config_path();

    ConfigStatus parse_cli_and_env(int argc, char** argv, Config& cfg);
    ConfigStatus parse_file(const std::string& path, Config& cfg);

    ConfigStatus check_platform_and_device(int platform_index, int device_index);
    ConfigStatus validate(const Config& cfg);

    ConfigStatus configure(int argc, char** argv);

}// namespace spla