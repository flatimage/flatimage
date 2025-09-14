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
      stderr=subprocess.PIPE,
      text=True,
      env=env or os.environ.copy()
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_root(self):
    # Simple command
    out,err,code = self.run_cmd("fim-root", "echo", "test")
    self.assertEqual(out, "test")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Check UID
    out,err,code = self.run_cmd("fim-root", "id", "-u")
    self.assertEqual(out, "0")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Check package installation
    out,err,code = self.run_cmd("fim-version-full")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.run_cmd("fim-perms", "set", "network")
    if "ALPINE" in out:
      print("Distribution is alpine")
      out,err,code = self.run_cmd("fim-root", "apk", "add", "curl")
      self.assertTrue("Installing curl" in out)
    elif "ARCH" in out:
      print("Distribution is arch")
      out,err,code = self.run_cmd("fim-root", "pacman", "-Sy", "--noconfirm", "curl")
      self.assertTrue("Overriding the desktop file MIME type cache..." in out)
    elif "BLUEPRINT":
      print("Distribution is blueprint")
    else:
      assert False, "Invalid distribution label"