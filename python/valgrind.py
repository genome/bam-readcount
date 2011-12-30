#!/usr/bin/env python

from subprocess import Popen, PIPE
import os
import re

class ValgrindWrapper:
    vg_path = None
    disable_var = "BCINT_TEST_NO_VALGRIND"

    def __init__(self, command, vglog_file):
        self.command = command
        self.vglog_file = vglog_file
        if self.vg_path == None:
            for d in os.environ["PATH"].split(os.pathsep):
                path = os.path.join(d, "valgrind")
                if os.path.exists(path) and os.access(path, os.X_OK):
                    self.vg_path = path
                    break

    def have_valgrind(self):
        return self.vg_path != None and os.getenv(self.disable_var) == None

    def run(self):
        cmd = self.command

        if self.have_valgrind():
            cmd[:0] = [
                "valgrind",
                "--error-exitcode=1",
                "--leak-check=full",
                "--log-file=%s" %self.vglog_file,
            ]

        p = Popen(cmd, stderr=PIPE, close_fds=True)
        out, err = p.communicate(None)
        if not self.leak_free():
            raise RuntimeError(
                "Possible memory leaks detected in command %s" %(" ".join(cmd))
            )
        return p.returncode, err

    def leak_free(self):
        if self.have_valgrind() == False:
            return True

        log_contents = open(self.vglog_file).read()
        m = re.search("no leaks are possible", log_contents)
        return m != None 
