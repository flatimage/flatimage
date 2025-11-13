#!/bin/python3
"""
Test suite for fim-env clear command.
"""

from .common import EnvTestBase
from cli.test_runner import run_cmd

class TestFimEnvClear(EnvTestBase):
  """
  Tests for fim-env clear command - removing all environment variables.
  """

  # === Clear Operation Tests ===

  def test_variable_clear(self):
    """Test clearing all environment variables."""
    # Define variables
    run_cmd(self.file_image, "fim-env", "add", "HELLO=WORLD", "TEST=ME")
    out,err,code = run_cmd(self.file_image, "fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 2)
    self.assertEqual(lines[0], "HELLO=WORLD")
    self.assertEqual(lines[1], "TEST=ME")
    # Clear
    out,err,code = run_cmd(self.file_image, "fim-env", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Extra argument
    out,err,code = run_cmd(self.file_image, "fim-env", "clear", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-env: ['foo',]", err)
    self.assertEqual(code, 125)
