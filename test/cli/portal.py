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

  def run_cmd(self, *args):
    result = subprocess.run(
      list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_portal(self):
    # FlatImage should not contain Jq
    out,err,code = self.run_cmd(str(self.file_image), "fim-exec", "command", "-v", "jq")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 1)
    # Host should contain Jq
    out,err,code = self.run_cmd("which", "jq")
    self.assertEqual(out, "/usr/bin/jq")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Execute host Jq through the portal facility
    # -r in jq to avoid double quotes around selected value
    process = subprocess.Popen(
      [self.file_image, "fim-exec", "fim_portal", "jq", "-r", ".key[1]"],
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    # Feed the JSON through stdin and close stdin to signal EOF
    stdout, stderr = process.communicate('{ "key": ["val1", "val2"] }')
    returncode = process.wait()
    self.assertEqual(stdout.strip(), "val2")
    self.assertEqual(stderr.strip(), "")
    self.assertEqual(returncode, 0)