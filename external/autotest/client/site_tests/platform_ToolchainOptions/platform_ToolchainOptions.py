# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import fnmatch
import glob
import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from optparse import OptionParser

FILE_CMD="file -m /usr/local/share/misc/magic.mgc"

class ToolchainOptionSet:
    """
    Handles a set of hits, along with potential whitelists to ignore.
    """
    def __init__(self, description, bad_files, whitelist_file):
        self.description = description
        self.bad_set = set(bad_files.splitlines())
        self.whitelist_set = set([])
        self.process_whitelist_with_private(whitelist_file)


    def process_whitelist_with_private(self, whitelist_file):
        """
        Filter out hits found on non-comment lines in the whitelist and
        and private whitelist.

        @param whitelist_file: path to whitelist file
        """
        whitelist_files = [whitelist_file]
        private_file = os.path.join(os.path.dirname(whitelist_file),
                                    "private_" +
                                    os.path.basename(whitelist_file))
        whitelist_files.append(private_file)
        self.process_whitelists(whitelist_files)


    def process_whitelist(self, whitelist_file):
        """
        Filter out hits found on non-comment lines in the whitelist.

        @param whitelist_file: path to whitelist file
        """
        if os.path.isfile(whitelist_file):
            f = open(whitelist_file)
            whitelist = [x for x in f.read().splitlines()
                                    if not x.startswith('#')]
            f.close()
            self.whitelist_set = self.whitelist_set.union(set(whitelist))

        filtered_list = []
        for bad_file in self.bad_set:
            # Does |bad_file| match any entry in the whitelist?
            in_whitelist = any([fnmatch.fnmatch(bad_file, whitelist_entry)
                                for whitelist_entry in self.whitelist_set])
            if not in_whitelist:
                filtered_list.append(bad_file)

        self.filtered_set = set(filtered_list)
        # TODO(jorgelo): remove glob patterns from |new_passes|.
        self.new_passes = self.whitelist_set.difference(self.bad_set)


    def process_whitelists(self, whitelist_files):
        """
        Filter out hits found in a list of whitelist files.

        @param whitelist_files: list of paths to whitelist files
        """
        for whitelist_file in whitelist_files:
            self.process_whitelist(whitelist_file)


    def get_fail_summary_message(self):
        m = "Test %s " % self.description
        m += "%d failures" % len(self.filtered_set)
        return m


    def get_fail_message(self):
        m = self.get_fail_summary_message()
        sorted_list = list(self.filtered_set)
        sorted_list.sort()
        m += "\nFAILED:\n%s\n\n" % "\n".join(sorted_list)
        return m


    def __str__(self):
        m = "Test %s " % self.description
        m += ("%d failures, %d in whitelist, %d in filtered, %d new passes " %
              (len(self.bad_set),
               len(self.whitelist_set),
               len(self.filtered_set),
               len(self.new_passes)))

        if len(self.filtered_set):
            sorted_list = list(self.filtered_set)
            sorted_list.sort()
            m += "FAILED:\n%s" % "\n".join(sorted_list)
        else:
            m += "PASSED!"

        if len(self.new_passes):
            sorted_list = list(self.new_passes)
            sorted_list.sort()
            m += ("\nNew passes (remove these from the whitelist):\n%s" %
                  "\n".join(sorted_list))
        logging.debug(m)
        return m


class platform_ToolchainOptions(test.test):
    """
    Tests for various expected conditions on ELF binaries in the image.
    """
    version = 2

    def get_cmd(self, test_cmd, find_options=""):
        base_cmd = ("find '%s' -wholename %s -prune -o "
                    " -wholename /proc -prune -o "
                    " -wholename /dev -prune -o "
                    " -wholename /sys -prune -o "
                    " -wholename /mnt/stateful_partition -prune -o "
                    " -wholename /usr/local -prune -o "
                    # There are files in /home/chronos that cause false
                    # positives, and since that's noexec anyways, it should
                    # be skipped.
                    " -wholename '/home/chronos' -prune -o "
                    " %s "
                    " -not -name 'libstdc++.so.*' "
                    " -not -name 'libgcc_s.so.*' "
                    " -type f -executable -exec "
                    "sh -c '%s "
                    "{} | grep -q ELF && "
                    "(%s || echo {})' ';'")
        rootdir = "/"
        cmd = base_cmd % (rootdir, self.autodir, find_options, FILE_CMD,
                          test_cmd)
        return cmd


    def create_and_filter(self, description, cmd, whitelist_file,
                          find_options=""):
        """
        Runs a command, with "{}" replaced (via "find -exec") with the
        target ELF binary. If the command fails, the file is marked as
        failing the test. Results are filtered against the provided
        whitelist file.

        @param description: text name of the check being done
        @param cmd: command to run via find's -exec option
        @param whitelist_file: list of failures to ignore
        @param find_options: additional options for find to limit the scope
        """
        full_cmd = self.get_cmd(cmd, find_options)
        bad_files = utils.system_output(full_cmd)
        cso = ToolchainOptionSet(description, bad_files, whitelist_file)
        cso.process_whitelist_with_private(whitelist_file)
        return cso


    def run_once(self, rootdir="/", args=[]):
        """
        Do a find for all the ELF files on the system.
        For each one, test for compiler options that should have been used
        when compiling the file.

        For missing compiler options, print the files.
        """

        parser = OptionParser()
        parser.add_option('--hardfp',
                          dest='enable_hardfp',
                          default=False,
                          action='store_true',
                          help='Whether to check for hardfp binaries.')
        (options, args) = parser.parse_args(args)

        option_sets = []

        libc_glob = "/lib/libc-[0-9]*"

        readelf_cmd = glob.glob("/usr/local/*/binutils-bin/*/readelf")[0]

        # We do not test binaries if they are built with Address Sanitizer
        # because it is a separate testing tool.
        no_asan_used = utils.system_output("%s -s "
                                           "/opt/google/chrome/chrome | "
                                           "egrep -q \"__asan_init\" || "
                                           "echo no ASAN" % readelf_cmd)
        if not no_asan_used:
            logging.debug("ASAN detected on /opt/google/chrome/chrome. "
                          "Will skip all checks.")
            return

        # Check that gold was used to build binaries.
        # TODO(jorgelo): re-enable this check once crbug.com/417912 is fixed.
        # gold_cmd = ("%s -S {} 2>&1 | "
        #             "egrep -q \".note.gnu.gold-ve\"" % readelf_cmd)
        # gold_find_options = ""
        # if utils.get_cpu_arch() == "arm":
        #     # gold is only enabled for Chrome on ARM.
        #     gold_find_options = "-path \"/opt/google/chrome/chrome\""
        # gold_whitelist = os.path.join(self.bindir, "gold_whitelist")
        # option_sets.append(self.create_and_filter("gold",
        #                                           gold_cmd,
        #                                           gold_whitelist,
        #                                           gold_find_options))

        # Verify non-static binaries have BIND_NOW in dynamic section.
        now_cmd = ("(%s {} | grep -q statically) ||"
                   "%s -d {} 2>&1 | "
                   "egrep -q \"BIND_NOW\"" % (FILE_CMD, readelf_cmd))
        if utils.is_freon():
            now_whitelist = os.path.join(self.bindir, "now_whitelist")
        else:
            now_whitelist = os.path.join(self.bindir, "now_whitelist_x")
        option_sets.append(self.create_and_filter("-Wl,-z,now",
                                                  now_cmd,
                                                  now_whitelist))

        # Verify non-static binaries have RELRO program header.
        relro_cmd = ("(%s {} | grep -q statically) ||"
                     "%s -l {} 2>&1 | "
                     "egrep -q \"GNU_RELRO\"" % (FILE_CMD, readelf_cmd))
        relro_whitelist = os.path.join(self.bindir, "relro_whitelist")
        option_sets.append(self.create_and_filter("-Wl,-z,relro",
                                                  relro_cmd,
                                                  relro_whitelist))

        # Verify non-static binaries are dynamic (built PIE).
        pie_cmd = ("(%s {} | grep -q statically) ||"
                   "%s -l {} 2>&1 | "
                   "egrep -q \"Elf file type is DYN\"" % (FILE_CMD,
                                                          readelf_cmd))
        pie_whitelist = os.path.join(self.bindir, "pie_whitelist")
        option_sets.append(self.create_and_filter("-fPIE",
                                                  pie_cmd,
                                                  pie_whitelist))

        # Verify all binaries have non-exec STACK program header.
        stack_cmd = ("%s -lW {} 2>&1 | "
                     "egrep -q \"GNU_STACK.*RW \"" % readelf_cmd)
        stack_whitelist = os.path.join(self.bindir, "stack_whitelist")
        option_sets.append(self.create_and_filter("Executable Stack",
                                                  stack_cmd,
                                                  stack_whitelist))

        # Verify all binaries have W^X LOAD program headers.
        loadwx_cmd = ("%s -lW {} 2>&1 | "
                      "grep \"LOAD\" | egrep -v \"(RW |R E)\" | "
                      "wc -l | grep -q \"^0$\"" % readelf_cmd)
        if utils.is_freon():
            loadwx_whitelist = os.path.join(self.bindir, "loadwx_whitelist")
        else:
            loadwx_whitelist = os.path.join(self.bindir, "loadwx_whitelist_x")
        option_sets.append(self.create_and_filter("LOAD Writable and Exec",
                                                  loadwx_cmd,
                                                  loadwx_whitelist))

        # Verify ARM binaries are all using VFP registers.
        if (options.enable_hardfp and utils.get_cpu_arch() == 'arm'):
            hardfp_cmd = ("%s -A {} 2>&1 | "
                          "egrep -q \"Tag_ABI_VFP_args: VFP registers\"" %
                          readelf_cmd)
            hardfp_whitelist = os.path.join(self.bindir, "hardfp_whitelist")
            option_sets.append(self.create_and_filter("hardfp", hardfp_cmd,
                                                      hardfp_whitelist))

        fail_msg = ""

        # There is currently no way to clear binary prebuilts for all devs.
        # Thus, when a new check is added to this test, the test might fail
        # for users who have old prebuilts which have not been compiled
        # in the correct manner.
        fail_summaries = []
        full_msg = "Test results:"
        num_fails = 0
        for cos in option_sets:
            if len(cos.filtered_set):
                num_fails += 1
                fail_msg += cos.get_fail_message() + "\n"
                fail_summaries.append(cos.get_fail_summary_message())
            full_msg += str(cos) + "\n\n"
        fail_summary_msg = ", ".join(fail_summaries)

        logging.error(fail_msg)
        logging.debug(full_msg)
        if num_fails:
            raise error.TestFail(fail_summary_msg)
