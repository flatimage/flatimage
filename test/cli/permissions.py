#!/bin/python3

import os
import shutil
import subprocess
import unittest
from pathlib import Path


class TestFimPerms(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.dir_script = Path(__file__).resolve().parent
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.home_custom = cls.dir_script / "user"

  def setUp(self):
    self.run_cmd("fim-perms", "clear")

  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_bind_and_del_home(self):
    self.home_custom.mkdir(parents=True, exist_ok=True)
    with open(self.home_custom / "file", "w") as f:
      f.write("secret\n")
    self.run_cmd("fim-perms", "add", "home")
    out,err,code = self.run_cmd("fim-exec", "cat", str(self.home_custom / "file"))
    self.assertEqual(out.strip(), "secret")
    self.assertEqual(len(out.strip().splitlines()), 1)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    self.run_cmd("fim-perms", "del", "home")
    out,err,code = self.run_cmd("fim-exec", "cat", str(self.home_custom / "file"))
    self.assertEqual(out, "")
    self.assertIn("No such file or directory", err)
    self.assertEqual(len(err.strip().splitlines()), 1)
    shutil.rmtree(self.home_custom)
    self.assertEqual(code, 1)

  def test_bind_network(self):
    out,err,code = self.run_cmd("fim-version-full")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.run_cmd("fim-perms", "add", "network")
    if "ALPINE" in out:
      self.run_cmd("fim-root", "apk", "add", "curl")
    elif "BLUEPRINT" in out:
      return
    out,err,code = self.run_cmd("fim-exec", "curl", "-sS", "https://am.i.mullvad.net/connected")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertIn("You are not connected to Mullvad", out)
    self.run_cmd("fim-perms", "del", "network")

  def test_multiple_permissions(self):
    self.run_cmd("fim-perms", "add", "home,network")
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("home", out)
    self.assertIn("network", out)
    self.assertGreaterEqual(len(lines), 2)
    self.run_cmd("fim-perms", "del", "home,network")
    self.assertEqual(out, "home\nnetwork")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_set_permissions(self):
    self.run_cmd("fim-perms", "add", "audio,wayland,xorg")
    self.run_cmd("fim-perms", "set", "input,gpu")
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("input", out)
    self.assertIn("gpu", out)
    self.assertGreaterEqual(len(lines), 2)

  def test_add(self):
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Missing permission
    out,err,code = self.run_cmd("fim-perms", "add")
    self.assertEqual(out, "")
    self.assertIn("No arguments for 'ADD' command", err)
    self.assertEqual(code, 125)
    # Invalid permission
    out,err,code = self.run_cmd("fim-perms", "add", "hello")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'HELLO'", err)
    self.assertEqual(code, 125)
    # Invalid permission mixed with valid permission
    out,err,code = self.run_cmd("fim-perms", "add", "home,hello")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'HELLO'", err)
    self.assertEqual(code, 125)
    # Trying to add 'none' as a permission
    out,err,code = self.run_cmd("fim-perms", "add", "none")
    self.assertEqual(out, "")
    self.assertIn("Invalid permission 'NONE'", err)
    self.assertEqual(code, 125)
    # Extra arguments
    out,err,code = self.run_cmd("fim-perms", "add", "home", "world")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", err)
    self.assertEqual(code, 125)
    # Argument add
    out,err,code = self.run_cmd("fim-perms", "add", "home,xorg,wayland,network")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual("home\nnetwork\nwayland\nxorg", out)
    # Add more permissions
    out,err,code = self.run_cmd("fim-perms", "add", "dbus_system,dbus_user")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual("dbus_system\ndbus_user\nhome\nnetwork\nwayland\nxorg", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Add all + others
    out,err,code = self.run_cmd("fim-perms", "add", "all,home")
    self.assertEqual(out, "")
    self.assertIn("Permission 'all' should not be used with others", err)
    self.assertEqual(code, 125)
    # Add all
    out,err,code = self.run_cmd("fim-perms", "add", "all")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual(
      "audio\ndbus_system\ndbus_user\ngpu\nhome\n"
      "input\nmedia\nnetwork\nudev\nusb\nwayland\nxorg"
      , out
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_list(self):
    # Set permissions
    self.run_cmd("fim-perms", "add", "audio,wayland,xorg")
    out,err,code = self.run_cmd("fim-perms", "list")
    lines = out.strip().splitlines()
    # List permissions
    self.assertIn("audio", out)
    self.assertIn("wayland", out)
    self.assertIn("xorg", out)
    self.assertGreaterEqual(len(lines), 3)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Extra arguments
    out,err,code = self.run_cmd("fim-perms", "list", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-perms: ['foo',]", err)
    self.assertEqual(code, 125)

  def test_clear(self):
    # Set permissions
    self.run_cmd("fim-perms", "add", "audio,wayland,xorg")
    out,err,code = self.run_cmd("fim-perms", "list")
    lines = out.strip().splitlines()
    self.assertGreaterEqual(len(lines), 3)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Clear permissions
    self.run_cmd("fim-perms", "clear")
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual(out, '')
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Extra arguments
    out,err,code = self.run_cmd("fim-perms", "clear", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-perms: ['foo',]", err)
    self.assertEqual(code, 125)

  def test_del(self):
    # Missing permission
    out,err,code = self.run_cmd("fim-perms", "del")
    self.assertIn("No arguments for 'DEL' command", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Invalid permission
    out,err,code = self.run_cmd("fim-perms", "del", "hello")
    self.assertIn("Could not determine enum entry from 'HELLO'", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Invalid permission mixed with valid permission
    out,err,code = self.run_cmd("fim-perms", "del", "home,hello")
    self.assertIn("Could not determine enum entry from 'HELLO'", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Extra arguments
    out,err,code = self.run_cmd("fim-perms", "del", "home", "world")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Trying to del 'none' permission
    out,err,code = self.run_cmd("fim-perms", "del", "none")
    self.assertIn("Invalid permission 'NONE'", err)
    self.assertEqual(out, "")
    self.assertEqual(code, 125)

  def test_set(self):
    # Missing permission
    out,err,code = self.run_cmd("fim-perms", "set")
    self.assertEqual(out, "")
    self.assertIn("No arguments for 'SET' command", err)
    self.assertEqual(code, 125)
    # Invalid permission
    out,err,code = self.run_cmd("fim-perms", "set", "hello")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'HELLO'", err)
    self.assertEqual(code, 125)
    # Invalid permission mixed with valid permission
    out,err,code = self.run_cmd("fim-perms", "set", "home,hello")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'HELLO'", err)
    self.assertEqual(code, 125)
    # Extra arguments
    out,err,code = self.run_cmd("fim-perms", "set", "home", "world")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", err)
    self.assertEqual(code, 125)
    # Trying to set 'none' permission
    out,err,code = self.run_cmd("fim-perms", "set", "none")
    self.assertEqual(out, "")
    self.assertIn("Invalid permission 'NONE'", err)
    self.assertEqual(code, 125)
    # Argument set
    out,err,code = self.run_cmd("fim-perms", "set", "home,xorg,wayland,network")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual("home\nnetwork\nwayland\nxorg", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Argument re-set
    out,err,code = self.run_cmd("fim-perms", "set", "dbus_system,dbus_user")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual("dbus_system\ndbus_user", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Set all + others
    out,err,code = self.run_cmd("fim-perms", "set", "all,home")
    self.assertEqual(out, "")
    self.assertIn("Permission 'all' should not be used with others", err)
    self.assertEqual(code, 125)
    # Set all
    out,err,code = self.run_cmd("fim-perms", "set", "all")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual(
      "audio\ndbus_system\ndbus_user\ngpu\nhome\n"
      "input\nmedia\nnetwork\nudev\nusb\nwayland\nxorg"
      , out
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-perms", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)