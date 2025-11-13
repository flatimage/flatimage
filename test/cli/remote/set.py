#!/bin/python3
"""
Test suite for fim-remote set command.
"""

from .common import RemoteTestBase
from cli.test_runner import run_cmd

class TestFimRemoteSet(RemoteTestBase):
  """
  Tests for fim-remote set command - configuring remote repository URLs.
  """

  # === Set Operation Tests ===

  def test_remote_set(self):
    """Test setting remote URLs."""
    # No URL argument to set
    out,err,code = run_cmd(self.file_image, "fim-remote", "set")
    self.assertEqual(out, "")
    self.assertIn("Missing URL for 'set' operation", err)
    self.assertEqual(code, 125)
    
    # Set a valid URL
    out,err,code = run_cmd(self.file_image, "fim-remote", "set", "https://example.com/updates")
    self.assertIn("Set remote URL to 'https://example.com/updates'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    # Verify it was set
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "https://example.com/updates")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    # Update to a different URL
    out,err,code = run_cmd(self.file_image, "fim-remote", "set", "https://mirror.example.org/repo")
    self.assertIn("Set remote URL to 'https://mirror.example.org/repo'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    # Verify the update
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "https://mirror.example.org/repo")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    # Trailing arguments to set
    out,err,code = run_cmd(self.file_image, "fim-remote", "set", "https://example.com", "extra")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-remote: ['extra',]", err)
    self.assertEqual(code, 125)
