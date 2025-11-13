#!/bin/python3

import json
from .common import BootTestBase
from cli.test_runner import run_cmd

class TestFimBootClear(BootTestBase):
  """
  Test suite for fim-boot clear command:
  - Clear boot configuration
  - Reset to default bash program
  - Validate clear behavior
  """

  # ===========================================================================
  # fim-boot clear Tests
  # ===========================================================================

  def test_boot_clear(self):
    """Test clear command resets boot configuration"""
    # Trailing arguments to clear
    out, err, code = run_cmd(self.file_image, "fim-boot", "clear", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-boot: ['hello',]", err)
    self.assertEqual(code, 125)
    def boot_show(stdout="", stderr="", code=0):
      # No arguments to show
      out, err, code = run_cmd(self.file_image, "fim-boot", "show")
      self.assertEqual(out, stdout)
      self.assertEqual(err, stderr)
      self.assertEqual(code, code)
    def boot_clear():
      out, err, code = run_cmd(self.file_image, "fim-boot", "clear")
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    # No arguments to show
    boot_show("""{\n  "args": [],\n  "program": "bash"\n}""")
    # Clear argument
    boot_clear()
    # Show default arguments
    boot_show("""{\n  "args": [],\n  "program": "bash"\n}""")
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
    boot_clear()
    # Show default arguments
    boot_show("""{\n  "args": [],\n  "program": "bash"\n}""")
