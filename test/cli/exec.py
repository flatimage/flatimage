#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib

class TestFimExec(unittest.TestCase):

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

  def test_exec(self):
    # Simple command
    out,err,code = self.run_cmd("fim-exec", "echo", "test")
    self.assertEqual(out, "test")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Check UID
    out,err,code = self.run_cmd("fim-exec", "id", "-u")
    self.assertEqual(out, str(os.getuid()))
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_exec_no_arguments(self):
    out,err,code = self.run_cmd("fim-exec")
    self.assertEqual(out, "")
    self.assertIn("Incorrect number of arguments for fim-exec", err)
    self.assertEqual(code, 125)