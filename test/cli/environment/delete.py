#!/bin/python3
"""
Test suite for fim-env del command.
"""

from .common import EnvTestBase
from cli.test_runner import run_cmd

class TestFimEnvDel(EnvTestBase):
  """
  Tests for fim-env del command - deleting specific environment variables.
  """

  # === Delete Operation Tests ===

  def test_variable_deletion(self):
    """Test deleting environment variables."""
    # No variables
    out,err,code = run_cmd(self.file_image, "fim-env", "del")
    self.assertEqual(out, "")
    self.assertIn("Missing arguments for 'DEL'", err)
    self.assertEqual(code, 125)
    # Multiple variables
    out,err,code = run_cmd(self.file_image, "fim-env", "set", "IMADE=THIS", "NO=IDID", "TEST=ME")
    self.assertIn("Included variable 'IMADE' with value 'THIS'", out)
    self.assertIn("Included variable 'NO' with value 'IDID'", out)
    self.assertIn("Included variable 'TEST' with value 'ME'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 3)
    # Deleted 2 variables
    out,err,code = run_cmd(self.file_image, "fim-env", "del", "IMADE", "NO")
    self.assertIn("Erase key 'IMADE'", out)
    self.assertIn("Erase key 'NO'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 1)
    self.assertEqual(lines[0], "TEST=ME")
    # Deleted all variables
    out,err,code = run_cmd(self.file_image, "fim-env", "del", "TEST")
    self.assertIn("Erase key 'TEST'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-env", "list")
    lines = out.splitlines()
    self.assertEqual(len(lines), 0)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
