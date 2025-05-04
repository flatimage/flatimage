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
    output = self.run_cmd("fim-root", "echo", "test")
    self.assertEqual(output, "test")
    output = self.run_cmd("fim-root", "sh", "-c" "cat /etc/*-release")
    self.run_cmd("fim-perms", "set", "network")
    if "Alpine" in output:
      output = self.run_cmd("fim-root", "apk", "add", "curl")
      self.assertTrue("Installing curl" in output)
    elif "Arch":
      output = self.run_cmd("fim-root", "pacman", "-Sy", "--noconfirm", "curl")
      self.assertTrue("Overriding the desktop file MIME type cache..." in output)