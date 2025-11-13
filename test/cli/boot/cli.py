#!/bin/python3

from .common import BootTestBase
from cli.test_runner import run_cmd

class TestFimBootCLI(BootTestBase):
  """
  Test suite for fim-boot CLI validation:
  - Command validation
  - Argument validation
  - Error handling for missing/invalid arguments
  """

  # ===========================================================================
  # CLI Validation Tests
  # ===========================================================================

  def test_boot_cli(self):
    """Test fim-boot CLI argument validation"""
    # No arguments to fim-boot
    out, err, code = run_cmd(self.file_image, "fim-boot")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-boot' (<set|show|clear>)", err)
    self.assertEqual(code, 125)
    # Invalid argument to fim-boot
    out, err, code = run_cmd(self.file_image, "fim-boot", "sett")
    self.assertEqual(out, "")
    self.assertIn("Invalid boot operation", err)
    self.assertEqual(code, 125)
