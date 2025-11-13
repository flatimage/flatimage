#!/bin/python3
"""
Test suite for fim-env CLI validation and variable format validation.
"""

from .common import EnvTestBase
from cli.test_runner import run_cmd

class TestFimEnvCli(EnvTestBase):
  """
  Tests for fim-env command line interface validation and invalid input handling.
  """

  # === CLI Validation Tests ===

  def test_variable_invalid(self):
    """Test invalid variable formats."""
    # Invalid variable
    out,err,code = run_cmd(self.file_image, "fim-env", "set", "IMADETHIS")
    self.assertEqual(out, "")
    self.assertIn("Variable assignment 'IMADETHIS' is invalid", err)
    self.assertEqual(code, 125)
    
    # Multiple invalid variables
    out,err,code = run_cmd(self.file_image, "fim-env", "set", "IMADETHIS", "HELLOWORLD")
    self.assertEqual(out, "")
    self.assertIn("Variable assignment 'IMADETHIS' is invalid", err)
    self.assertEqual(code, 125)

  def test_option_empty(self):
    """Test missing operation."""
    # Empty variable
    out,err,code = run_cmd(self.file_image, "fim-env")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-env' (add,del,list,set,clear)", err)
    self.assertEqual(code, 125)

  def test_option_invalid(self):
    """Test invalid operation."""
    # Invalid variable
    out,err,code = run_cmd(self.file_image, "fim-env", "addd")
    self.assertEqual(out, "")
    self.assertIn("Invalid env operation", err)
    self.assertEqual(code, 125)
