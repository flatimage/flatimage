#!/bin/python3
"""
Test suite for fim-env list command.
"""

from .common import EnvTestBase
from cli.test_runner import run_cmd

class TestFimEnvList(EnvTestBase):
  """
  Tests for fim-env list command - displaying environment variables.
  """

  # === List Operation Tests ===

  def test_variable_list(self):
    """Test listing environment variables."""
    # No variables
    out,err,code = run_cmd(self.file_image, "fim-env", "list")
    self.assertEqual("", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Multiple variables
    run_cmd(self.file_image, "fim-env", "add", "HELLO=WORLD", "TEST=ME")
    out,err,code = run_cmd(self.file_image, "fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 2)
    self.assertEqual(lines[0], "HELLO=WORLD")
    self.assertEqual(lines[1], "TEST=ME")
    # Extra argument
    out,err,code = run_cmd(self.file_image, "fim-env", "list", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-env: ['foo',]", err)
    self.assertEqual(code, 125)
