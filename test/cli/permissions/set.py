#!/bin/python3

import unittest
from .common import PermsTestBase
from cli.test_runner import run_cmd

class TestFimPermsSet(PermsTestBase):
  """
  Test suite for fim-perms set command:
  - Set permissions (replacing existing)
  - Handle 'all' permission
  - Validate set command arguments
  """

  # ===========================================================================
  # fim-perms set Tests
  # ===========================================================================

  def test_set_permissions(self):
    """Test set command replaces existing permissions"""
    run_cmd(self.file_image, "fim-perms", "add", "audio,wayland,xorg")
    run_cmd(self.file_image, "fim-perms", "set", "input,gpu")
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("input", out)
    self.assertIn("gpu", out)
    self.assertGreaterEqual(len(lines), 2)

  def test_set(self):
    """Test set command with various inputs"""
    # Missing permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "set")
    self.assertEqual(out, "")
    self.assertIn("No arguments for 'SET' command", err)
    self.assertEqual(code, 125)
    # Invalid permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "set", "hello")
    self.assertEqual(out, "")
    self.assertIn("Invalid permission", err)
    self.assertEqual(code, 125)
    # Invalid permission mixed with valid permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "set", "home,hello")
    self.assertEqual(out, "")
    self.assertIn("Invalid permission", err)
    self.assertEqual(code, 125)
    # Extra arguments
    out, err, code = run_cmd(self.file_image, "fim-perms", "set", "home", "world")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", err)
    self.assertEqual(code, 125)
    # Trying to set 'none' permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "set", "none")
    self.assertEqual(out, "")
    self.assertIn("Invalid permission 'NONE'", err)
    self.assertEqual(code, 125)
    # Argument set
    out, err, code = run_cmd(self.file_image, "fim-perms", "set", "home,xorg,wayland,network")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual("home\nnetwork\nwayland\nxorg", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Argument re-set
    out, err, code = run_cmd(self.file_image, "fim-perms", "set", "dbus_system,dbus_user")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual("dbus_system\ndbus_user", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Set all + others
    out, err, code = run_cmd(self.file_image, "fim-perms", "set", "all,home")
    self.assertEqual(out, "")
    self.assertIn("Permission 'all' should not be used with others", err)
    self.assertEqual(code, 125)
    # Set all
    out, err, code = run_cmd(self.file_image, "fim-perms", "set", "all")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual(
      "audio\ndbus_system\ndbus_user\ndev\ngpu\nhome\n"
      "input\nmedia\nnetwork\noptical\nshm\nudev\nusb\nwayland\nxorg"
      , out
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-perms", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
