#!/usr/bin/env python

from glob import glob
from subprocess import Popen, PIPE
from valgrindwrapper import ValgrindWrapper
import difflib
import os
import re
import shlex
import shutil
import sys
import tempfile
import unittest

class IntegrationTest():
    exe_path = None
    def setUp(self):
        self.test_dir = os.path.dirname(sys.argv[0])
        self.data_dir = os.path.join(self.test_dir, "data")
        self.tmp_dir = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.tmp_dir)

    def inputFiles(self, *names):
        rv = []
        for n in names:
            rv.extend(sorted(glob(os.path.join(self.data_dir, n))))
        if len(rv) == 0:
            raise IOError("No file matching %s not found in %s" %(
                ", ".join(names),
                self.data_dir)
            )
        return rv

    def execute(self, args):
        cmdline = "%s %s" %(self.exe_path, " ".join(args))
        vglog_file = self.tempFile("valgrind.log")
        return ValgrindWrapper(shlex.split(cmdline), vglog_file).run()

    def tempFile(self, name):
        return os.path.join(self.tmp_dir, name)

    def assertFilesEqual(self, first, second, msg=None, filter_regex=None):
        first_data = open(first).readlines()
        second_data = open(second).readlines()
        if filter_regex:
            first_data = [x for x in first_data if not re.match(filter_regex, x)]
            second_data = [x for x in second_data if not re.match(filter_regex, x)]
        self.assertMultiLineEqual("".join(first_data), "".join(second_data))

        
    def assertMultiLineEqual(self, first, second, msg=None):
        """Assert that two multi-line strings are equal.
        If they aren't, show a nice diff.
        """
        self.assertTrue(isinstance(first, str),
                'First argument is not a string')
        self.assertTrue(isinstance(second, str),
                'Second argument is not a string')

        if first != second:
            message = ''.join(difflib.ndiff(first.splitlines(True),
                                                second.splitlines(True)))
            if msg:
                message += " : " + msg
            self.fail("Multi-line strings are unequal:\n" + message)

def main():
    if len(sys.argv) < 2:
        print "Error: required argument (path to test executable) missing"
        sys.exit(1)
    IntegrationTest.exe_path = sys.argv.pop()
    unittest.main()
