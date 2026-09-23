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

#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "CLI/CLI.hpp"
#include <nlohmann/json.hpp>

#ifdef _WIN32
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
    #include <pwd.h>
    #include <unistd.h>
#endif

namespace spla {

    struct Profile {
        std::optional<int>                      platform;
        std::optional<int>                      device;
        std::optional<int>                      queues;
        std::optional<bool>                     profiling;
        std::optional<std::string>              allocator;
        std::optional<size_t>                   allocator_size;
        std::optional<int>                      verbosity;
        std::optional<std::vector<std::string>> extends;

        void merge(const Profile& source);
    };

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

        std::optional<std::string>                    profile;
        std::optional<std::map<std::string, Profile>> profiles;

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

        ProfileNotFound,
        ProfileCycle,

        MissedParameters,
        PlatformNotFound,
        DeviceNotFound,
        InvalidConfigParams
    };

    extern Config config_default;
    extern Config config_system;
    extern Config config_user;
    extern Config config_cli_and_env;
    extern Config config_final;

    std::string get_spla_version();

    std::string get_default_config_path();
    std::string get_default_system_config_path();
    std::string get_home_directory();
    std::string get_default_user_config_path();

    ConfigStatus parse_cli_and_env(int argc, char** argv, Config& cfg);
    ConfigStatus parse_file(const std::string& path, Config& cfg);

    ConfigStatus check_platform_and_device(int platform_index, int device_index);
    ConfigStatus validation(const Config& cfg);

    Profile      apply_extends(const std::map<std::string, Profile>& profiles,
                               const std::string&                    name,
                               std::set<std::string>&                stack);
    ConfigStatus apply_profile(Config& cfg);

    ConfigStatus configure(int argc, char** argv);

}// namespace spla