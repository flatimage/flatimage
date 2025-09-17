#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib

class TestFimCasefold(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def tearDown(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)
    self.run_cmd("fim-casefold", "off")
    
  def run_cmd(self, *args, env=None):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
      env=env or os.environ.copy()
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_casefold_disabled(self):
    out,err,code = self.run_cmd("fim-exec", "mkdir", "-p", "/hElLo/WoRlD")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-exec", "stat", "/hello/world")
    self.assertEqual(out, "")
    self.assertIn("No such file or directory", err)
    self.assertEqual(code, 1)

  def test_casefold_enabled(self):
    os.environ["FIM_OVERLAY"] = "unionfs"
    out,err,code = self.run_cmd("fim-casefold", "on")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-exec", "mkdir", "-p", "/hElLo/WoRlD")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-exec", "stat", "/hello/world")
    self.assertIn("File: /hello/world", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-exec", "stat", "/hEllO/WorlD")
    self.assertIn("File: /hEllO/WorlD", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)