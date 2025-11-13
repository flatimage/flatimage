#!/bin/python3

import unittest
from .common import PermsTestBase
from cli.test_runner import run_cmd

class TestFimPermsList(PermsTestBase):
  """
  Test suite for fim-perms list command:
  - List active permissions
  - Handle empty permission lists
  - Validate list command arguments
  """

  # ===========================================================================
  # fim-perms list Tests
  # ===========================================================================

  def test_list(self):
    """Test list command"""
    # Set permissions
    run_cmd(self.file_image, "fim-perms", "add", "audio,wayland,xorg")
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    lines = out.strip().splitlines()
    # List permissions
    self.assertIn("audio", out)
    self.assertIn("wayland", out)
    self.assertIn("xorg", out)
    self.assertGreaterEqual(len(lines), 3)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Extra arguments
    out, err, code = run_cmd(self.file_image, "fim-perms", "list", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-perms: ['foo',]", err)
    self.assertEqual(code, 125)
