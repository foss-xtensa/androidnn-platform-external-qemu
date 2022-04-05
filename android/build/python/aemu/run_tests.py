#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import os
import platform
import shutil
import subprocess
import sys
import tempfile
import logging

from aemu.definitions import EXE_POSTFIX, get_ctest, get_qemu_root
from aemu.process import run


class TemporaryDirectory(object):
    """Creates a temporary directory that will be deleted when moving out of scope"""

    def __enter__(self):
        self.name = tempfile.mkdtemp()
        return self.name

    def __exit__(self, exc_type, exc_value, traceback):
        # Unfortunately there are a lot of unicode files that python doesn't handle properly
        # so we will let windows handle it..
        if platform.system() == "Windows":
            run(["rmdir", "/s", "/q", self.name])
        else:
            shutil.rmtree(self.name)


def run_tests(out_dir, jobs, check_symbols, additional_opts):
    if check_symbols:
        run_symbols_test(out_dir)

    if platform.system() == "Windows":

        if "--skip-emulator-check" in additional_opts:
            pass
        else:
            run_binary_exists(out_dir)

        run_emugen_test(out_dir)
        run_ctest(out_dir, jobs)
    else:
        with TemporaryDirectory() as tmpdir:
            logging.info("Running tests with TMPDIR=%s", tmpdir)
            run(
                [
                    os.path.join(
                        get_qemu_root(), "android", "scripts", "unix", "run_tests.sh"
                    ),
                    "--out-dir=%s" % out_dir,
                    "--verbose",
                    "--verbose",
                    "-j",
                    jobs,
                ]
                + additional_opts,
                out_dir,
                {"TMPDIR": tmpdir},
            )


def run_symbols_test(out_dir):
    """Test to make sure we can upload symbols to our symbol server."""
    API_KEY_FILE = os.path.join(os.path.expanduser("~"), ".emulator_symbol_server_key")
    small_sym_file = os.path.join(out_dir, "build", "debug_info", "mksdcard.sym")
    upload_exe = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "upload_symbols.py"
    )
    assert os.path.exists(small_sym_file)

    if not os.path.exists(API_KEY_FILE):
        logging.error(
            "----------------#### .emulator_symbol_server_key not installed ###--------------------------"
        )
        logging.error("Unable to validate if symbols are actually processed.")

    # This will explode if we return a non 0 status
    subprocess.check_call(
        [
            sys.executable,
            upload_exe,
            "--symbol_file",
            small_sym_file,
            "--environment",
            "prod",
        ]
    )
    subprocess.check_call(
        [
            sys.executable,
            upload_exe,
            "--symbol_file",
            small_sym_file,
            "--environment",
            "staging",
        ]
    )


def run_binary_exists(out_dir):
    if not os.path.isfile(os.path.join(out_dir, "emulator%s" % EXE_POSTFIX)):
        raise Exception("Emulator binary is missing")


def run_ctest(out_dir, jobs):
    cmd = [get_ctest(), "-j", jobs, "--output-on-failure"]
    with TemporaryDirectory() as tmpdir:
        logging.info("Running tests with TMP=%s", tmpdir)
        run(cmd, out_dir, {"TMP": tmpdir, "TEMP": tmpdir})


def run_emugen_test(out_dir):
    emugen = os.path.abspath(os.path.join(out_dir, "emugen%s" % EXE_POSTFIX))
    if platform.system() != "Windows":
        cmd = [
            os.path.join(
                get_qemu_root(),
                "android",
                "android-emugl",
                "host",
                "tools",
                "emugen",
                "tests",
                "run-tests.sh",
            ),
            "--emugen=%s" % emugen,
        ]
        run(cmd, out_dir)
    else:
        logging.info("gen_tests not supported on windows yet.")
