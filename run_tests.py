##################################################################################
# This file is part of spla project                                              #
# https://github.com/SparseLinearAlgebra/spla                                    #
##################################################################################
# MIT License                                                                    #
#                                                                                #
# Copyright (c) 2023 SparseLinearAlgebra                                         #
#                                                                                #
# Permission is hereby granted, free of charge, to any person obtaining a copy   #
# of this software and associated documentation files (the "Software"), to deal  #
# in the Software without restriction, including without limitation the rights   #
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      #
# copies of the Software, and to permit persons to whom the Software is          #
# furnished to do so, subject to the following conditions:                       #
#                                                                                #
# The above copyright notice and this permission notice shall be included in all #
# copies or substantial portions of the Software.                                #
#                                                                                #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    #
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  #
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  #
# SOFTWARE.                                                                      #
##################################################################################

import shared
import argparse
import subprocess
import re


def test_names(test_src_path):
    cmake_file_path = test_src_path / "CMakeLists.txt"
    with open(cmake_file_path) as cmake_file:
        all_tests = re.findall(r"spla_test_target\((\w+)\)", cmake_file.read())
    return all_tests


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--spla-config", default=None,
                        help="Name of the spla configuration to apply (e.g., test_cpu, test_gpu)")
    parser.add_argument("--spla-config-file", default=None,
                        help="Path to a spla configuration file")
    args = parser.parse_args()

    tests_dir = shared.ROOT / args.build_dir / "tests"
    print(f"Searching for unit-tests in `{tests_dir}` folder")

    # Build the spla-config flags to pass to each test binary
    config_flags = []
    if args.spla_config_file:
        config_flags.extend(["--spla-config-file", args.spla_config_file])
    if args.spla_config:
        config_flags.extend(["--spla-config", args.spla_config])

    if args.spla_config:
        print(f"Using spla configuration: {args.spla_config}")
    if args.spla_config_file:
        print(f"Using spla configuration file: {args.spla_config_file}")

    failed_tests = []
    skipped_tests = []
    all_tests = test_names(shared.ROOT / "tests")
    for test_name in all_tests:
        full_test_path = str(tests_dir / test_name)
        cmd = [full_test_path] + config_flags
        try:
            subprocess.check_call(cmd)
            print(f"Exec unit-test: `{full_test_path}`")
        except FileNotFoundError:
            skipped_tests.append(test_name)
            print(f"Skipped: `{full_test_path}`")
        except subprocess.CalledProcessError:
            failed_tests.append(test_name)
            print(f"Failed: `{full_test_path}`")

    all_tests_string = '\n\t'.join(all_tests)
    print(f"All executed tests: \n\t{all_tests_string}")

    if skipped_tests:
        skipped_tests_string = '\n\t'.join(skipped_tests)
        print(f"Skipped tests: \n\t{skipped_tests_string}")

    if failed_tests:
        failed_tests_string = '\n\t'.join(failed_tests)
        raise Exception(f"Some tests have not been passed: \n\t{failed_tests_string}")


if __name__ == "__main__":
    main()
