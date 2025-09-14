#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib

class TestFimBoot(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image_src = os.environ["FILE_IMAGE_SRC"]
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def setUp(self):
    shutil.copyfile(self.file_image_src, self.file_image)

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

  def test_boot_cli(self):
    # No arguments to fim-boot
    out,err,code = self.run_cmd("fim-boot")
    self.assertEqual(out, "")
    self.assertIn("Invalid operation for 'fim-boot' (<set|show|clear>)", err)
    self.assertEqual(code, 125)
    # Invalid argument to fim-boot
    out,err,code = self.run_cmd("fim-boot", "sett")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'SETT'", err)
    self.assertEqual(code, 125)

  def test_boot_set(self):
    # No arguments to fim-boot
    out,err,code = self.run_cmd("fim-boot")
    self.assertEqual(out, "")
    self.assertIn("Invalid operation for 'fim-boot' (<set|show|clear>)", err)
    self.assertEqual(code, 125)
    # No arguments to set
    out,err,code = self.run_cmd("fim-boot", "set", "echo")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Argument passing
    out,err,code = self.run_cmd("fim-boot", "set", "echo")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("test")
    self.assertEqual(out, "test")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Built-in arguments
    self.run_cmd("fim-boot", "set", "echo", "hello world")
    out,err,code = self.run_cmd()
    self.assertEqual(out, "hello world")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Built-in arguments + cli arguments
    out,err,code = self.run_cmd("folks people")
    self.assertEqual(out, "hello world folks people")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Argument passing through bash
    self.run_cmd("fim-boot", "set", "bash", "-c")
    out,err,code = self.run_cmd("""echo "arg1: $1 and arg2: $2\"""", "/bin/bash", "fst", "snd")
    self.assertEqual(out, "arg1: fst and arg2: snd")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_boot_show(self):
    # Trailing arguments to show
    out,err,code = self.run_cmd("fim-boot", "show", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-boot: ['hello',]", err)
    self.assertEqual(code, 125)
    def boot_show():
      # No arguments to show
      out,err,code = self.run_cmd("fim-boot", "show")
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    # No arguments to show
    boot_show()
    # Set argument
    out,err,code = self.run_cmd("fim-boot", "set", "bash", "-c", """echo "hello" "world\"""")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-boot", "show")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    import json
    boot = json.loads(out)
    self.assertEqual(boot["program"], "bash")
    self.assertEqual(boot["args"], ["-c", """echo "hello" "world\""""])
    # Clear argument
    out,err,code = self.run_cmd("fim-boot", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # No arguments to show
    boot_show()

  def test_boot_clear(self):
    # Trailing arguments to clear
    out,err,code = self.run_cmd("fim-boot", "clear", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-boot: ['hello',]", err)
    self.assertEqual(code, 125)
    def boot_show():
      # No arguments to show
      out,err,code = self.run_cmd("fim-boot", "show")
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    def boot_clear():
      out,err,code = self.run_cmd("fim-boot", "clear")
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    # No arguments to show
    boot_show()
    # Clear argument
    boot_clear()
    # No arguments to show
    boot_show()
    # Set argument
    out,err,code = self.run_cmd("fim-boot", "set", "bash", "-c", """echo "hello" "world\"""")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-boot", "show")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    import json
    boot = json.loads(out)
    self.assertEqual(boot["program"], "bash")
    self.assertEqual(boot["args"], ["-c", """echo "hello" "world\""""])
    # Clear argument
    boot_clear()
    # No arguments to show
    boot_show()