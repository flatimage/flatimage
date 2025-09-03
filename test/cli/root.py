#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib

class TestFimRoot(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def tearDown(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)
    
  def run_cmd(self, *args, env=None):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True,
      env=env or os.environ.copy()
    )
    return result.stdout.strip()

  def test_root(self):
    # Simple command
    output = self.run_cmd("fim-root", "echo", "test")
    self.assertEqual(output, "test")
    # Check UID
    output = self.run_cmd("fim-root", "id", "-u")
    self.assertEqual(output, "0")
    # Check package installation
    output = self.run_cmd("fim-version-full")
    self.run_cmd("fim-perms", "set", "network")
    if "ALPINE" in output:
      print("Distribution is alpine")
      output = self.run_cmd("fim-root", "apk", "add", "curl")
      self.assertTrue("Installing curl" in output)
    elif "ARCH" in output:
      print("Distribution is arch")
      output = self.run_cmd("fim-root", "pacman", "-Sy", "--noconfirm", "curl")
      self.assertTrue("Overriding the desktop file MIME type cache..." in output)
    elif "BLUEPRINT":
      print("Distribution is blueprint")
    else:
      assert False, "Invalid distribution label"