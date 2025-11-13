#!/bin/python3
"""
Test suite for fim-remote clear command.
"""

from .common import RemoteTestBase
from cli.test_runner import run_cmd

class TestFimRemoteClear(RemoteTestBase):
  """
  Tests for fim-remote clear command - removing remote URL configuration.
  """

  # === Clear Operation Tests ===

  def test_remote_clear(self):
    """Test clearing remote URLs."""
    # Clear when no URL is set
    out,err,code = run_cmd(self.file_image, "fim-remote", "clear")
    self.assertIn("Cleared remote URL", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    # Verify it's still empty
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "")
    self.assertIn("No remote URL configured", err)
    self.assertEqual(code, 125)
    
    # Set a URL
    out,err,code = run_cmd(self.file_image, "fim-remote", "set", "https://repo.example.com/images")
    self.assertIn("Set remote URL to 'https://repo.example.com/images'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    # Verify it was set
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "https://repo.example.com/images")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    # Clear the URL
    out,err,code = run_cmd(self.file_image, "fim-remote", "clear")
    self.assertIn("Cleared remote URL", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    # Verify it was cleared
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "")
    self.assertIn("No remote URL configured", err)
    self.assertEqual(code, 125)
    
    # Trailing arguments to clear
    out,err,code = run_cmd(self.file_image, "fim-remote", "clear", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-remote: ['hello',]", err)
    self.assertEqual(code, 125)
