#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib

class TestFimEnv(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def setUp(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def tearDown(self):
    self.assertTrue(pathlib.Path(self.dir_image).exists())
    
  def run_cmd(self, *args, env=None):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True,
      env=env or os.environ.copy()
    )
    return result.stdout.strip()

  def test_variable_add(self):
    self.run_cmd("fim-env", "add", "HELLO=WORLD", "TEST=ME")
    hello = self.run_cmd("fim-exec", "sh", "-c", "echo $HELLO")
    test = self.run_cmd("fim-exec", "sh", "-c", "echo $TEST")
    self.assertEqual(hello, "WORLD")
    self.assertEqual(test, "ME")

  def test_variable_list(self):
    self.run_cmd("fim-env", "add", "HELLO=WORLD", "TEST=ME")
    output = self.run_cmd("fim-env", "list")
    lines = output.splitlines()
    self.assertEqual(len(lines), 2)
    self.assertEqual(lines[0], "HELLO=WORLD")
    self.assertEqual(lines[1], "TEST=ME")

  def test_variable_set(self):
    self.run_cmd("fim-env", "add", "HELLO=WORLD", "TEST=ME")
    self.run_cmd("fim-env", "set", "IMADE=THIS", "NO=IDID")
    output = self.run_cmd("fim-env", "list")
    lines = output.splitlines()
    self.assertEqual(len(lines), 2)
    self.assertEqual(lines[0], "IMADE=THIS")
    self.assertEqual(lines[1], "NO=IDID")

  def test_variable_deletion(self):
    self.run_cmd("fim-env", "set", "IMADE=THIS", "NO=IDID", "TEST=ME")
    output = self.run_cmd("fim-env", "list")
    lines = output.splitlines()
    self.assertEqual(len(lines), 3)
    self.run_cmd("fim-env", "del", "IMADE", "NO")
    output = self.run_cmd("fim-env", "list")
    lines = output.splitlines()
    self.assertEqual(len(lines), 1)
    self.assertEqual(lines[0], "TEST=ME")
    self.run_cmd("fim-env", "del", "TEST")
    output = self.run_cmd("fim-env", "list")
    lines = output.splitlines()
    self.assertEqual(len(lines), 0)

  def test_runtime_defined_variables(self):
    self.run_cmd("fim-env", "add", '$(echo $META)=WORLD', 'TEST=$(echo $SOMETHING)')
    runtime_env = os.environ.copy()
    runtime_env["META"] = "NAME"
    runtime_env["SOMETHING"] = "ELSE"
    output = self.run_cmd("fim-env", "list", env=runtime_env)
    lines = output.splitlines()
    self.assertEqual(lines[0], "NAME=WORLD")
    self.assertEqual(lines[1], "TEST=ELSE")