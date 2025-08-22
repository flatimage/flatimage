#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib

class TestFimBoot(unittest.TestCase):

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

  def test_boot(self):
    # No arguments
    output = self.run_cmd("fim-boot")
    self.assertIn("Incorrect number of arguments for 'fim-boot' (<program> [args...])", output)
    # Simple command
    self.run_cmd("fim-boot", "echo")
    output = self.run_cmd("test")
    self.assertEqual(output, "test")
    # Command with arguments
    self.run_cmd("fim-boot", "echo", "hello world")
    output = self.run_cmd()
    self.assertEqual(output, "hello world")
    # Built-in arguments + cli arguments
    output = self.run_cmd("folks people")
    self.assertEqual(output, "hello world folks people")
    # Argument passing through bash
    self.run_cmd("fim-boot", "bash", "-c")
    output = self.run_cmd("""echo "arg1: $1 and arg2: $2\"""", "/bin/bash", "fst", "snd")
    self.assertEqual(output, "arg1: fst and arg2: snd")