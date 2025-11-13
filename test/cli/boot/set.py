#!/bin/python3

from .common import BootTestBase
from cli.test_runner import run_cmd

class TestFimBootSet(BootTestBase):
  """
  Test suite for fim-boot set command:
  - Set boot program and arguments
  - Handle argument passing
  - Configure boot command with various programs
  """

  # ===========================================================================
  # fim-boot set Tests
  # ===========================================================================

  def test_boot_set(self):
    """Test set command with various configurations"""
    # No arguments to fim-boot
    out, err, code = run_cmd(self.file_image, "fim-boot")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-boot' (<set|show|clear>)", err)
    self.assertEqual(code, 125)
    # No arguments to set
    out, err, code = run_cmd(self.file_image, "fim-boot", "set", "echo")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Argument passing
    out, err, code = run_cmd(self.file_image, "fim-boot", "set", "echo")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "test")
    self.assertEqual(out, "test")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Built-in arguments
    run_cmd(self.file_image, "fim-boot", "set", "echo", "hello world")
    out, err, code = run_cmd(self.file_image, )
    self.assertEqual(out, "hello world")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Built-in arguments + cli arguments
    out, err, code = run_cmd(self.file_image, "folks people")
    self.assertEqual(out, "hello world folks people")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Argument passing through bash
    run_cmd(self.file_image, "fim-boot", "set", "bash", "-c")
    out, err, code = run_cmd(self.file_image, """echo "arg1: $1 and arg2: $2\"""", "/bin/bash", "fst", "snd")
    self.assertEqual(out, "arg1: fst and arg2: snd")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
