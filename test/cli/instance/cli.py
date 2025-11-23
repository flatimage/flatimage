#!/bin/python3

import time
from .common import InstanceTestBase
from cli.test_runner import run_cmd, spawn_cmd

class TestFimInstanceCli(InstanceTestBase):
  """Test suite for fim-instance CLI argument validation"""

  def test_cli(self):
    """Test CLI argument validation"""
    # Missing arguments for instance
    out,err,code = run_cmd(self.file_image, "fim-instance")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-instance' (<exec|list>)", err)
    self.assertEqual(code, 125)
    # Missing arguments for exec
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec")
    self.assertEqual(out, "")
    self.assertIn("Missing 'id' argument for 'fim-instance'", err)
    self.assertEqual(code, 125)
    # Invalid id
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "foo")
    self.assertEqual(out, "")
    self.assertIn("Id argument must be a digit", err)
    self.assertEqual(code, 125)
    # Missing instance
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "0", "echo", "hello")
    self.assertEqual(out, "")
    self.assertIn("No instances are running", err)
    self.assertEqual(code, 125)
    # Missing instance command
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "0")
    self.assertEqual(out, "")
    self.assertIn("Missing 'cmd' argument for 'fim-instance'", err)
    self.assertEqual(code, 125)
    # Out of bounds id
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "-1", "echo", "hello")
    self.assertEqual(out, "")
    self.assertIn("Id argument must be a digit", err)
    self.assertEqual(code, 125)
    out,err,code = run_cmd(self.file_image, "fim-instance", "exec", "1", "echo", "hello")
    self.assertEqual(out, "")
    self.assertIn("No instances are running", err)
    self.assertEqual(code, 125)