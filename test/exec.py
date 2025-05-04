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
      stderr=subprocess.STDOUT,
      text=True,
      env=env or os.environ.copy()
    )
    return result.stdout.strip()

  def test_exec(self):
    output = self.run_cmd("fim-exec", "echo", "test")
    self.assertEqual(output, "test")