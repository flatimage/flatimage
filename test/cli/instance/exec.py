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
    # Spawn command as root
    self.procs.append(spawn_cmd(self.file_image, "fim-root", "sleep", "10"))
    time.sleep(1)
    # Spawn command as regular user
    self.procs.append(spawn_cmd(self.file_image, "fim-exec", "sleep", "10"))
    time.sleep(1)
    # Test root id
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "0", "id", "-u")
    self.assertEqual(out, "0")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Test regular user id
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "1", "id", "-u")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertEqual(out, str(os.getuid()))
    [proc.kill() for proc in self.procs]
    time.sleep(1)
