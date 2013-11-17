#!/usr/bin/env python
"""This is a simple python script to perform an integration
test on bam-readcount. It uses some small BAMs from sequencing
of NA19238 and NA19240 and the integrationtest.py from
TGI's build-common submodule."""

import os
print "I AM IN", os.getcwd()
from integrationtest import IntegrationTest, main
from testdata import TEST_DATA_DIRECTORY
import unittest
import subprocess
import os

class TestBamReadcount(IntegrationTest, unittest.TestCase):

    def setUp(self):
        IntegrationTest.setUp(self)
        self.data_dir = TEST_DATA_DIRECTORY
        self.orig_path = os.path.realpath(os.getcwd())
        self.exe_path = os.path.realpath(self.exe_path)
        os.chdir(self.data_dir)

    def tearDown(self):
        IntegrationTest.tearDown(self)
        os.chdir(self.orig_path)

#    def test_bamreadcount_normal(self):
#        expected_file = "expected_output.cn_per_lib"
#        config_file = "inv_del_bam_config"
#        output_file = self.tempFile("output")
#        cmdline = " ".join([self.exe_path, '-a', '-o', '21', config_file, '>', output_file])
#        print "Executing", cmdline
#        print "CWD", os.getcwd()
#        #params = [ "-o 21", " > ", output_file ]
#        #rv, err = self.execute_through_shell(params)
#        rv = subprocess.call(cmdline, shell=True)
#        print "Return value:", rv
#        self.assertEqual(0, rv)
#        self.assertFilesEqual(expected_file, output_file, filter_regex="#Command|#Software")
    def test_bamreadcount_perlib(self):
        expected_file = "expected__per_lib"
        site_list = "site_list"
        output_file = self.tempFile("output")
        cmdline = " ".join([self.exe_path, '-l', site_list, '>', output_file])
        print "Executing", cmdline
        print "CWD", os.getcwd()
        rv = subprocess.call(cmdline, shell=True)
        print "Return value:", rv
        self.assertEqual(0, rv)
        self.assertFilesEqual(expected_file, output_file) 

if __name__ == "__main__":
    main()
