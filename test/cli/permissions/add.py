#!/bin/python3

import unittest
import shutil
from .common import PermsTestBase
from cli.test_runner import run_cmd

class TestFimPermsAdd(PermsTestBase):
  """
  Test suite for fim-perms add command:
  - Add individual permissions
  - Add multiple permissions
  - Handle 'all' permission
  - Validate permission names
  """

  # ===========================================================================
  # fim-perms add Tests
  # ===========================================================================

  def test_bind_and_del_home(self):
    """Test adding and deleting home permission"""
    self.home_custom.mkdir(parents=True, exist_ok=True)
    with open(self.home_custom / "file", "w") as f:
      f.write("secret\n")
    run_cmd(self.file_image, "fim-perms", "add", "home")
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", str(self.home_custom / "file"))
    self.assertEqual(out.strip(), "secret")
    self.assertEqual(len(out.strip().splitlines()), 1)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    run_cmd(self.file_image, "fim-perms", "del", "home")
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", str(self.home_custom / "file"))
    self.assertEqual(out, "")
    self.assertIn("No such file or directory", err)
    self.assertEqual(len(err.strip().splitlines()), 1)
    shutil.rmtree(self.home_custom)
    self.assertEqual(code, 1)

  def test_bind_network(self):
    """Test adding network permission"""
    out, err, code = run_cmd(self.file_image, "fim-version", "full")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    run_cmd(self.file_image, "fim-perms", "add", "network")
    if "ALPINE" in out:
      run_cmd(self.file_image, "fim-root", "apk", "add", "curl")
    elif "BLUEPRINT" in out:
      return
    out, err, code = run_cmd(self.file_image, "fim-exec", "curl", "-sS", "https://am.i.mullvad.net/connected")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertIn("You are not connected to Mullvad", out)
    run_cmd(self.file_image, "fim-perms", "del", "network")

  def test_bind_dev(self):
    """Test adding dev permission"""
    import subprocess
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "dev")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    files_container, err, code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "ls -1 /dev | sort -d")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    result = subprocess.run(
      ["sh", "-c", "ls -1 /dev | sort -d"],
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    files_host = result.stdout.strip()
    self.assertEqual(result.stderr, "")
    self.assertEqual(result.returncode, 0)
    self.assertEqual(files_container, files_host)

  def test_multiple_permissions(self):
    """Test adding multiple permissions at once"""
    run_cmd(self.file_image, "fim-perms", "add", "home,network")
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("home", out)
    self.assertIn("network", out)
    self.assertGreaterEqual(len(lines), 2)
    run_cmd(self.file_image, "fim-perms", "del", "home,network")
    self.assertEqual(out, "home\nnetwork")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_add(self):
    """Test add command with various inputs"""
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Missing permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "add")
    self.assertEqual(out, "")
    self.assertIn("No arguments for 'ADD' command", err)
    self.assertEqual(code, 125)
    # Invalid permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "hello")
    self.assertEqual(out, "")
    self.assertIn("Invalid permission", err)
    self.assertEqual(code, 125)
    # Invalid permission mixed with valid permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "home,hello")
    self.assertEqual(out, "")
    self.assertIn("Invalid permission", err)
    self.assertEqual(code, 125)
    # Trying to add 'none' as a permission
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "none")
    self.assertEqual(out, "")
    self.assertIn("Invalid permission 'NONE'", err)
    self.assertEqual(code, 125)
    # Extra arguments
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "home", "world")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", err)
    self.assertEqual(code, 125)
    # Argument add
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "home,xorg,wayland,network")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual("home\nnetwork\nwayland\nxorg", out)
    # Add more permissions
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "dbus_system,dbus_user")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-perms", "list")
    self.assertEqual("dbus_system\ndbus_user\nhome\nnetwork\nwayland\nxorg", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Add all + others
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "all,home")
    self.assertEqual(out, "")
    self.assertIn("Permission 'all' should not be used with others", err)
    self.assertEqual(code, 125)
    # Add all
    out, err, code = run_cmd(self.file_image, "fim-perms", "add", "all")
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
