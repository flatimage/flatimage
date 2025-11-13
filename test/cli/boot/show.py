#!/bin/python3

import json
from .common import BootTestBase
from cli.test_runner import run_cmd

class TestFimBootShow(BootTestBase):
  """
  Test suite for fim-boot show command:
  - Display current boot configuration
  - Show program and arguments
  - Handle different program types
  """

  # ===========================================================================
  # fim-boot show Tests
  # ===========================================================================

  def test_boot_show(self):
    """Test show command displays boot configuration"""
    # Trailing arguments to show
    out, err, code = run_cmd(self.file_image, "fim-boot", "show", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-boot: ['hello',]", err)
    self.assertEqual(code, 125)
    def boot_show():
      # No arguments to show
      out, err, code = run_cmd(self.file_image, "fim-boot", "show")
      self.assertEqual(out, """{\n  "args": [],\n  "program": "bash"\n}""")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    # No arguments to show
    boot_show()
    # Set argument
    out, err, code = run_cmd(self.file_image, "fim-boot", "set", "bash", "-c", """echo "hello" "world\"""")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-boot", "show")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    boot = json.loads(out)
    self.assertEqual(boot["program"], "bash")
    self.assertEqual(boot["args"], ["-c", """echo "hello" "world\""""])
    # Clear argument
    out, err, code = run_cmd(self.file_image, "fim-boot", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # No arguments to show
    boot_show()

  def test_boot_show_non_bash_program(self):
    """Test show command with non-bash programs"""
    # Set a non-bash program (echo) with arguments
    out, err, code = run_cmd(self.file_image, "fim-boot", "set", "echo", "hello", "world")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify that show displays the correct program (echo, not bash)
    out, err, code = run_cmd(self.file_image, "fim-boot", "show")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    boot = json.loads(out)
    self.assertEqual(boot["program"], "echo")
    self.assertEqual(boot["args"], ["hello", "world"])

    # Set a different program (firefox) with different arguments
    out, err, code = run_cmd(self.file_image, "fim-boot", "set", "firefox", "--no-remote", "--private-window")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify that show displays firefox (not bash or echo)
    out, err, code = run_cmd(self.file_image, "fim-boot", "show")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    boot = json.loads(out)
    self.assertEqual(boot["program"], "firefox")
    self.assertEqual(boot["args"], ["--no-remote", "--private-window"])

    # Clear the boot configuration
    out, err, code = run_cmd(self.file_image, "fim-boot", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # After clearing, show should display default bash
    out, err, code = run_cmd(self.file_image, "fim-boot", "show")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    boot = json.loads(out)
    self.assertEqual(boot["program"], "bash")
    self.assertEqual(boot["args"], [])
