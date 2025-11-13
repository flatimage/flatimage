#!/bin/python3
"""
Test suite for fim-env add command.
"""

import os
from .common import EnvTestBase
from cli.test_runner import run_cmd

class TestFimEnvAdd(EnvTestBase):
  """
  Tests for fim-env add command - adding environment variables.
  """
  # === Add Operation Tests ===

  def test_variable_add(self):
    """Test adding environment variables."""
    # No variables
    out,err,code = run_cmd(self.file_image, "fim-env", "add")
    self.assertEqual(out, "")
    self.assertIn("Missing arguments for 'ADD'", err)
    self.assertEqual(code, 125)
    # Multiple variables
    run_cmd(self.file_image, "fim-env", "add", "HELLO=WORLD", "TEST=ME")
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo $HELLO")
    self.assertEqual(out, "WORLD")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo $TEST")
    self.assertEqual(out, "ME")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_runtime_defined_variables(self):
    """Test adding variables with runtime evaluation."""
    out,err,code = run_cmd(self.file_image, "fim-env", "add", '$(echo $META)=WORLD', 'TEST=$(echo $SOMETHING)')
    self.assertIn("Included variable '$(echo $META)' with value 'WORLD'", out)
    self.assertIn("Included variable 'TEST' with value '$(echo $SOMETHING)'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    
    runtime_env = os.environ.copy()
    runtime_env["META"] = "NAME"
    runtime_env["SOMETHING"] = "ELSE"
    out,err,code = run_cmd(self.file_image, "fim-env", "list", env=runtime_env)
    lines = out.splitlines()
    self.assertEqual(lines[0], "NAME=WORLD")
    self.assertEqual(lines[1], "TEST=ELSE")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
