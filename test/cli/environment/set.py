#!/bin/python3
"""
Test suite for fim-env set command.
"""

from .common import EnvTestBase
from cli.test_runner import run_cmd

class TestFimEnvSet(EnvTestBase):
  """
  Tests for fim-env set command - replacing all environment variables.
  """

  # === Set Operation Tests ===

  def test_variable_set(self):
    """Test replacing all environment variables."""
    # No variables
    out,err,code = run_cmd(self.file_image, "fim-env", "set")
    self.assertEqual(out, "")
    self.assertIn("Missing arguments for 'SET'", err)
    self.assertEqual(code, 125)
    # Multiple variables
    run_cmd(self.file_image, "fim-env", "add", "HELLO=WORLD", "TEST=ME")
    run_cmd(self.file_image, "fim-env", "set", "IMADE=THIS", "NO=IDID")
    out,err,code = run_cmd(self.file_image, "fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 2)
    self.assertEqual(lines[0], "IMADE=THIS")
    self.assertEqual(lines[1], "NO=IDID")
