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

  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    )
    return result.stdout.strip()

  def test_bind_and_del_home(self):
    self.home_custom.mkdir(parents=True, exist_ok=True)
    with open(self.home_custom / "file", "w") as f:
      f.write("secret\n")
    self.run_cmd("fim-perms", "add", "home")
    output = self.run_cmd("fim-exec", "cat", str(self.home_custom / "file"))
    self.assertEqual(output.strip(), "secret")
    self.assertEqual(len(output.strip().splitlines()), 1)

    self.run_cmd("fim-perms", "del", "home")
    output = self.run_cmd("fim-exec", "cat", str(self.home_custom / "file"))
    self.assertIn("No such file or directory", output)
    self.assertEqual(len(output.strip().splitlines()), 1)
    shutil.rmtree(self.home_custom)

  def test_bind_network(self):
    output = self.run_cmd("fim-version-full")
    self.run_cmd("fim-perms", "add", "network")
    if "ALPINE" in output:
      self.run_cmd("fim-root", "apk", "add", "curl")
    elif "BLUEPRINT" in output:
      return
    output = self.run_cmd("fim-exec", "curl", "-sS", "https://am.i.mullvad.net/connected")
    self.assertIn("You are not connected to Mullvad", output)
    self.run_cmd("fim-perms", "del", "network")

  def test_multiple_permissions(self):
    self.run_cmd("fim-perms", "add", "home,network")
    output = self.run_cmd("fim-perms", "list")
    lines = output.strip().splitlines()
    self.assertIn("home", output)
    self.assertIn("network", output)
    self.assertGreaterEqual(len(lines), 2)
    self.run_cmd("fim-perms", "del", "home,network")

  def test_set_permissions(self):
    self.run_cmd("fim-perms", "add", "audio,wayland,xorg")
    self.run_cmd("fim-perms", "set", "input,gpu")
    output = self.run_cmd("fim-perms", "list")
    lines = output.strip().splitlines()
    self.assertIn("input", output)
    self.assertIn("gpu", output)
    self.assertGreaterEqual(len(lines), 2)

  def test_add_cli(self):
    # Missing permission
    output = self.run_cmd("fim-perms", "add")
    self.assertIn("No arguments for 'ADD' command", output)
    # Invalid permission
    output = self.run_cmd("fim-perms", "add", "hello")
    self.assertIn("No match for 'hello'", output)
    # Invalid permission mixed with valid permission
    output = self.run_cmd("fim-perms", "add", "home,hello")
    self.assertIn("No match for 'hello'", output)
    # Extra arguments
    output = self.run_cmd("fim-perms", "add", "home", "world")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", output)

  def test_list_cli(self):
    # Set permissions
    self.run_cmd("fim-perms", "add", "audio,wayland,xorg")
    output = self.run_cmd("fim-perms", "list")
    lines = output.strip().splitlines()
    # List permissions
    self.assertIn("audio", output)
    self.assertIn("wayland", output)
    self.assertIn("xorg", output)
    self.assertGreaterEqual(len(lines), 3)
    # Extra arguments
    output = self.run_cmd("fim-perms", "list", "foo")
    self.assertIn("Trailing arguments for fim-perms: ['foo',]", output)

  def test_clear_cli(self):
    # Set permissions
    self.run_cmd("fim-perms", "add", "audio,wayland,xorg")
    output = self.run_cmd("fim-perms", "list")
    lines = output.strip().splitlines()
    self.assertGreaterEqual(len(lines), 3)
    # Clear permissions
    self.run_cmd("fim-perms", "clear")
    output = self.run_cmd("fim-perms", "list")
    self.assertEqual(output, '')
    # Extra arguments
    output = self.run_cmd("fim-perms", "clear", "foo")
    self.assertIn("Trailing arguments for fim-perms: ['foo',]", output)

  def test_del_cli(self):
    # Missing permission
    output = self.run_cmd("fim-perms", "del")
    self.assertIn("No arguments for 'DEL' command", output)
    # Invalid permission
    output = self.run_cmd("fim-perms", "del", "hello")
    self.assertIn("No match for 'hello'", output)
    # Invalid permission mixed with valid permission
    output = self.run_cmd("fim-perms", "del", "home,hello")
    self.assertIn("No match for 'hello'", output)
    # Extra arguments
    output = self.run_cmd("fim-perms", "del", "home", "world")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", output)

  def test_set_cli(self):
    # Missing permission
    output = self.run_cmd("fim-perms", "set")
    self.assertIn("No arguments for 'SET' command", output)
    # Invalid permission
    output = self.run_cmd("fim-perms", "set", "hello")
    self.assertIn("No match for 'hello'", output)
    # Invalid permission mixed with valid permission
    output = self.run_cmd("fim-perms", "set", "home,hello")
    self.assertIn("No match for 'hello'", output)
    # Extra arguments
    output = self.run_cmd("fim-perms", "set", "home", "world")
    self.assertIn("Trailing arguments for fim-perms: ['world',]", output)