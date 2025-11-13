#!/bin/python3

import unittest
from .common import PermsTestBase
from cli.test_runner import run_cmd

class TestFimPermsClear(PermsTestBase):
  """
  Test suite for fim-perms clear command:
  - Clear all permissions
  - Validate clear command behavior
  """

  # ===========================================================================
  # fim-perms clear Tests
  # ===========================================================================

  def test_clear(self):
    """Test clear command"""
    # Set permissions
    run_cmd(self.file_image, "fim-perms", "add", "audio,wayland,xorg")
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    lines = out.strip().splitlines()
    self.assertGreaterEqual(len(lines), 3)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Clear permissions
    run_cmd(self.file_image, "fim-perms", "clear")
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual(out, '')
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Extra arguments
    out, err, code = run_cmd(self.file_image, "fim-perms", "clear", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-perms: ['foo',]", err)
    self.assertEqual(code, 125)
