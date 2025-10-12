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
    shutil.copy(os.environ["FILE_IMAGE_SRC"], os.environ["FILE_IMAGE"])

  def tearDown(self):
    self.assertTrue(pathlib.Path(self.dir_image).exists())
    
  def run_cmd(self, *args, env=None):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
      env=env or os.environ.copy()
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_variable_add(self):
    # No variables
    out,err,code = self.run_cmd("fim-env", "add")
    self.assertEqual(out, "")
    self.assertIn("Missing arguments for 'ADD'", err)
    self.assertEqual(code, 125)
    # Multiple variables
    self.run_cmd("fim-env", "add", "HELLO=WORLD", "TEST=ME")
    out,err,code = self.run_cmd("fim-exec", "sh", "-c", "echo $HELLO")
    self.assertEqual(out, "WORLD")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-exec", "sh", "-c", "echo $TEST")
    self.assertEqual(out, "ME")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_variable_list(self):
    # No variables
    out,err,code = self.run_cmd("fim-env", "list")
    self.assertEqual("", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Multiple variables
    self.run_cmd("fim-env", "add", "HELLO=WORLD", "TEST=ME")
    out,err,code = self.run_cmd("fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 2)
    self.assertEqual(lines[0], "HELLO=WORLD")
    self.assertEqual(lines[1], "TEST=ME")
    # Extra argument
    out,err,code = self.run_cmd("fim-env", "list", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-env: ['foo',]", err)
    self.assertEqual(code, 125)

  def test_variable_clear(self):
    # Define variables
    self.run_cmd("fim-env", "add", "HELLO=WORLD", "TEST=ME")
    out,err,code = self.run_cmd("fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 2)
    self.assertEqual(lines[0], "HELLO=WORLD")
    self.assertEqual(lines[1], "TEST=ME")
    # Clear
    out,err,code = self.run_cmd("fim-env", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Extra argument
    out,err,code = self.run_cmd("fim-env", "clear", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-env: ['foo',]", err)
    self.assertEqual(code, 125)

  def test_variable_set(self):
    # No variables
    out,err,code = self.run_cmd("fim-env", "set")
    self.assertEqual(out, "")
    self.assertIn("Missing arguments for 'SET'", err)
    self.assertEqual(code, 125)
    # Multiple variables
    self.run_cmd("fim-env", "add", "HELLO=WORLD", "TEST=ME")
    self.run_cmd("fim-env", "set", "IMADE=THIS", "NO=IDID")
    out,err,code = self.run_cmd("fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 2)
    self.assertEqual(lines[0], "IMADE=THIS")
    self.assertEqual(lines[1], "NO=IDID")

  def test_variable_deletion(self):
    # No variables
    out,err,code = self.run_cmd("fim-env", "del")
    self.assertEqual(out, "")
    self.assertIn("Missing arguments for 'DEL'", err)
    self.assertEqual(code, 125)
    # Multiple variables
    out,err,code = self.run_cmd("fim-env", "set", "IMADE=THIS", "NO=IDID", "TEST=ME")
    self.assertIn("Included variable 'IMADE' with value 'THIS'", out)
    self.assertIn("Included variable 'NO' with value 'IDID'", out)
    self.assertIn("Included variable 'TEST' with value 'ME'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 3)
    # Deleted 2 variables
    out,err,code = self.run_cmd("fim-env", "del", "IMADE", "NO")
    self.assertIn("Erase key 'IMADE'", out)
    self.assertIn("Erase key 'NO'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-env", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.splitlines()
    self.assertEqual(len(lines), 1)
    self.assertEqual(lines[0], "TEST=ME")
    # Deleted all variables
    out,err,code = self.run_cmd("fim-env", "del", "TEST")
    self.assertIn("Erase key 'TEST'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-env", "list")
    lines = out.splitlines()
    self.assertEqual(len(lines), 0)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_runtime_defined_variables(self):
    out,err,code = self.run_cmd("fim-env", "add", '$(echo $META)=WORLD', 'TEST=$(echo $SOMETHING)')
    self.assertIn("Included variable '$(echo $META)' with value 'WORLD'", out)
    self.assertIn("Included variable 'TEST' with value '$(echo $SOMETHING)'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    runtime_env = os.environ.copy()
    runtime_env["META"] = "NAME"
    runtime_env["SOMETHING"] = "ELSE"
    out,err,code = self.run_cmd("fim-env", "list", env=runtime_env)
    lines = out.splitlines()
    self.assertEqual(lines[0], "NAME=WORLD")
    self.assertEqual(lines[1], "TEST=ELSE")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_variable_invalid(self):
    # Invalid variable
    out,err,code = self.run_cmd("fim-env", "set", "IMADETHIS")
    self.assertEqual(out, "")
    self.assertIn("Variable assignment 'IMADETHIS' is invalid", err)
    self.assertEqual(code, 125)
    # Multiple invalid variables
    out,err,code = self.run_cmd("fim-env", "set", "IMADETHIS", "HELLOWORLD")
    self.assertEqual(out, "")
    self.assertIn("Variable assignment 'IMADETHIS' is invalid", err)
    self.assertEqual(code, 125)

  def test_option_empty(self):
    # Empty variable
    out,err,code = self.run_cmd("fim-env")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-env' (add,del,list,set,clear)", err)
    self.assertEqual(code, 125)

  def test_option_invalid(self):
    # Invalid variable
    out,err,code = self.run_cmd("fim-env", "addd")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'ADDD'", err)
    self.assertEqual(code, 125)