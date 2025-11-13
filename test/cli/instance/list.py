#!/bin/python3

import re
import time
from .common import InstanceTestBase
from cli.test_runner import run_cmd, spawn_cmd

class TestFimInstanceList(InstanceTestBase):
  """Test suite for fim-instance list command"""
    
  def test_instances_list(self):
    """Test listing active instances"""
    # Spawn command as root
    self.procs.append(spawn_cmd(self.file_image, "fim-root", "sleep", "10"))
    time.sleep(1)
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
    self.assertEqual(len(re.compile(r"""(?m)^(\d+):(\d+)$""").findall(out)), 1)
    # Spawn command as regular user
    self.procs.append(spawn_cmd(self.file_image, "fim-exec", "sleep", "10"))
    time.sleep(1)
    # Check for multiple instances
    out,err,code = run_cmd(self.file_image, "fim-instance", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertEqual(len(re.compile(r"""(?m)^(\d+):(\d+)$""").findall(out)), 2)
    self.assertEqual(out.count('\n'), 1)
    [proc.kill() for proc in self.procs]
    time.sleep(1)
