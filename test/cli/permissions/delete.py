#!/bin/python3

import unittest
from .common import PermsTestBase
from cli.test_runner import run_cmd

class TestFimPermsDel(PermsTestBase):
  """
  Test suite for fim-perms del command:
  - Delete individual permissions
  - Delete multiple permissions
  - Validate deletion arguments
  """

  # ===========================================================================
  # fim-perms del Tests
  # ===========================================================================

  def test_del(self):
    """Test del command with various inputs"""
    # Missing permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "del")
    self.assertIn("No arguments for 'DEL' command", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Invalid permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "del", "hello")
    self.assertIn("Invalid permission", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Invalid permission mixed with valid permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "del", "home,hello")
    self.assertIn("Invalid permission", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Extra arguments
    out, err, code = run_cmd(self.file_image, "fim-perms", "del", "home", "world")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Trying to del 'none' permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "del", "none")
    self.assertIn("Invalid permission 'NONE'", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
