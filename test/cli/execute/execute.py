#!/bin/python3

import os
from .common import ExecTestBase
from cli.test_runner import run_cmd

class TestFimExec(ExecTestBase):
  """
  Base class for environment tests. Provides common setup/teardown and utilities for testing fim-env.
  """

  def test_exec(self):
    # Simple command
    out,err,code = run_cmd(self.file_image, "fim-exec", "echo", "test")
    self.assertEqual(out, "test")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Check UID
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(out, str(os.getuid()))
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_exec_no_arguments(self):
    out,err,code = run_cmd(self.file_image, "fim-exec")
    self.assertEqual(out, "")
    self.assertIn("Incorrect number of arguments for fim-exec", err)
    self.assertEqual(code, 125)