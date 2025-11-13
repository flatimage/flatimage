#!/bin/python3
"""
Test suite for fim-remote CLI validation.
"""

from .common import RemoteTestBase
from cli.test_runner import run_cmd

class TestFimRemoteCli(RemoteTestBase):
  """
  Tests for fim-remote command line interface validation.
  """

  # === CLI Validation Tests ===

  def test_remote_cli(self):
    """Test CLI argument validation for fim-remote."""
    # No arguments to fim-remote
    out,err,code = run_cmd(self.file_image, "fim-remote")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-remote' (<set|show|clear>)", err)
    self.assertEqual(code, 125)
    
    # Invalid argument to fim-remote
    out,err,code = run_cmd(self.file_image, "fim-remote", "sett")
    self.assertEqual(out, "")
    self.assertIn("Invalid remote operation", err)
    self.assertEqual(code, 125)
