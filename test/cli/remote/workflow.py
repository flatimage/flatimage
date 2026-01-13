#!/bin/python3
"""
Test suite for fim-remote workflow integration.
"""

from .common import RemoteTestBase
from cli.test_runner import run_cmd

class TestFimRemoteWorkflow(RemoteTestBase):
  """
  Tests for complete fim-remote workflows - end-to-end scenarios.
  """

  # === Workflow Integration Tests ===

  def test_remote_workflow(self):
    """Test complete remote configuration workflow."""
    # Complete workflow: set -> show -> clear -> show

    # Initial state be the default repository
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "https://raw.githubusercontent.com/flatimage/recipes/master")
    self.assertEqual(code, 0)

    # Set a remote URL
    out,err,code = run_cmd(self.file_image, "fim-remote", "set", "https://download.flatimage.io/stable")
    self.assertEqual(code, 0)

    # Verify it's set
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "https://download.flatimage.io/stable")
    self.assertEqual(code, 0)

    # Clear it
    out,err,code = run_cmd(self.file_image, "fim-remote", "clear")
    self.assertEqual(code, 0)

    # Verify it's cleared
    out,err,code = run_cmd(self.file_image, "fim-remote", "show")
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
