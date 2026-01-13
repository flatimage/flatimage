#!/bin/python3
"""
Test suite for fim-remote show command.
"""

from .common import RemoteTestBase
from cli.test_runner import run_cmd

class TestFimRemoteShow(RemoteTestBase):
  """
  Tests for fim-remote show command - displaying configured remote URLs.
  """

  # === Show Operation Tests ===

  def test_remote_show(self):
    """Test displaying remote URLs."""
    # Show default URL
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "https://raw.githubusercontent.com/flatimage/recipes/master")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Set a URL
    out,err,code = run_cmd(self.file_image, "fim-remote", "set", "https://cdn.example.net/flatimage")
    self.assertIn("Set remote URL to 'https://cdn.example.net/flatimage'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Show the URL
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "https://cdn.example.net/flatimage")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Trailing arguments to show
    out,err,code = run_cmd(self.file_image, "fim-remote", "show", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-remote: ['hello',]", err)
    self.assertEqual(code, 125)
