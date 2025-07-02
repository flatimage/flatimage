#!/bin/python3

import os
import shutil
import subprocess
import unittest
from pathlib import Path


class TestFimPortal(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]

  def test_portal(self):
    # FlatImage should not contain Jq
    returncode = subprocess.run(
      [self.file_image] + ["fim-exec", "which", "jq"],
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    ).returncode
    self.assertEqual(returncode, 1)
    # Host should contain Jq
    returncode = subprocess.run(
      ["which", "jq"],
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    ).returncode
    self.assertEqual(returncode, 0)
    # Execute host Jq through the portal facility
    # -r in jq to avoid double quotes around selected value
    process = subprocess.Popen(
      [self.file_image, "fim-exec", "fim_portal", "jq", "-r", ".key[1]"],
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    )
    # Feed the JSON through stdin and close stdin to signal EOF
    stdout, _ = process.communicate('{ "key": ["val1", "val2"] }')
    returncode = process.wait()
    self.assertEqual(stdout.strip(), "val2")
    self.assertEqual(returncode, 0)