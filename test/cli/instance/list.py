#!/bin/python3

import os
import re
import time
from .common import InstanceTestBase
from cli.test_runner import run_cmd, spawn_cmd

class TestFimInstanceList(InstanceTestBase):
  """Test suite for fim-instance list command"""

  def test_instances_list(self):
    """Test listing active instances"""
    os.environ["FIM_OVERLAY"] = "unionfs"
    procs = []
    # Extra argument
    out,err,code = run_cmd(self.file_image, "fim-instance", "list", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-instance: ['foo',]", err)
    self.assertEqual(code, 125)
    # Check for the instance value
    out,err,code = run_cmd(self.file_image, "fim-instance", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertEqual(out.count('\n'), 0)
    # Spawn command as root
    procs.append(spawn_cmd(self.file_image, "fim-root", "sleep", "5"))
    time.sleep(1)
    # Spawn command as regular user
    procs.append(spawn_cmd(self.file_image, "fim-exec", "sleep", "5"))
    time.sleep(1)
    # Check for multiple instances
    out,err,code = run_cmd(self.file_image, "fim-instance", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertEqual(out.count('\n'), 1)
    self.assertEqual(len(re.compile(r"""(?m)^(\d+):(\d+)$""").findall(out)), 2)
    [proc.kill() for proc in procs]
    time.sleep(1)
    del os.environ["FIM_OVERLAY"]
