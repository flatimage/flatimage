#!/bin/python3

import os
import re
import time
from .common import InstanceTestBase
from cli.test_runner import run_cmd, spawn_cmd

class TestFimInstanceExec(InstanceTestBase):
  """Test suite for fim-instance exec command"""

  def test_instances_exec(self):
    """Test executing commands in specific instances"""
    procs = []
    os.environ["FIM_OVERLAY"] = "unionfs"
    # Spawn command as root
    procs.append(spawn_cmd(self.file_image, "fim-root", "sleep", "5"))
    time.sleep(1)
    # Spawn command as regular user
    procs.append(spawn_cmd(self.file_image, "fim-exec", "sleep", "5"))
    time.sleep(1)
    # Test root id
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "0", "id", "-u")
    self.assertEqual(out, "0")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Test regular user id
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "1", "id", "-u")
    self.assertEqual(out, str(os.getuid()))
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    [proc.kill() for proc in procs]
    time.sleep(1)
    del os.environ["FIM_OVERLAY"]