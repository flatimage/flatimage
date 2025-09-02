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
      stderr=subprocess.STDOUT,
      text=True,
      env=env or os.environ.copy()
    )
    return (result.returncode, result.stdout.strip())

  def test_boot_cli(self):
    # No arguments to fim-boot
    ret, output = self.run_cmd("fim-boot")
    self.assertIn("Invalid operation for 'fim-boot' (<set|show|clear>)", output)
    self.assertEqual(ret, 125)
    # Invalid argument to fim-boot
    ret, output = self.run_cmd("fim-boot", "sett")
    self.assertIn("Could not determine enum entry from 'SETT'", output)
    self.assertEqual(ret, 125)

  def test_boot_set(self):
    # No arguments to fim-boot
    ret, output = self.run_cmd("fim-boot")
    self.assertIn("Invalid operation for 'fim-boot' (<set|show|clear>)", output)
    self.assertEqual(ret, 125)
    # No arguments to set
    ret, output = self.run_cmd("fim-boot", "set", "echo")
    self.assertEqual(output, "")
    self.assertEqual(ret, 0)
    # Argument passing
    ret, output = self.run_cmd("fim-boot", "set", "echo")
    self.assertEqual(output, "")
    self.assertEqual(ret, 0)
    ret, output = self.run_cmd("test")
    self.assertEqual(output, "test")
    # Built-in arguments
    self.run_cmd("fim-boot", "set", "echo", "hello world")
    ret, output = self.run_cmd()
    self.assertEqual(output, "hello world")
    # Built-in arguments + cli arguments
    ret, output = self.run_cmd("folks people")
    self.assertEqual(output, "hello world folks people")
    # Argument passing through bash
    self.run_cmd("fim-boot", "set", "bash", "-c")
    ret, output = self.run_cmd("""echo "arg1: $1 and arg2: $2\"""", "/bin/bash", "fst", "snd")
    self.assertEqual(output, "arg1: fst and arg2: snd")

  def test_boot_show(self):
    # Trailing arguments to show
    ret, output = self.run_cmd("fim-boot", "show", "hello")
    self.assertIn("Trailing arguments for fim-boot: ['hello',]", output)
    self.assertEqual(ret, 125)
    def boot_show():
      # No arguments to show
      ret, output = self.run_cmd("fim-boot", "show")
      self.assertEqual(output, "")
      self.assertEqual(ret, 0)
    # No arguments to show
    boot_show()
    # Set argument
    ret, output = self.run_cmd("fim-boot", "set", "bash", "-c", """echo "hello" "world\"""")
    self.assertEqual(output, "")
    self.assertEqual(ret, 0)
    ret, output = self.run_cmd("fim-boot", "show")
    self.assertEqual(ret, 0)
    import json
    boot = json.loads(output)
    self.assertEqual(boot["program"], "bash")
    self.assertEqual(boot["args"], ["-c", """echo "hello" "world\""""])
    # Clear argument
    ret, output = self.run_cmd("fim-boot", "clear")
    self.assertEqual(output, "")
    self.assertEqual(ret, 0)
    # No arguments to show
    boot_show()

  def test_boot_clear(self):
    # Trailing arguments to clear
    ret, output = self.run_cmd("fim-boot", "clear", "hello")
    self.assertIn("Trailing arguments for fim-boot: ['hello',]", output)
    self.assertEqual(ret, 125)
    def boot_show():
      # No arguments to show
      ret, output = self.run_cmd("fim-boot", "show")
      self.assertEqual(output, "")
      self.assertEqual(ret, 0)
    def boot_clear():
      ret, output = self.run_cmd("fim-boot", "clear")
      self.assertEqual(output, "")
      self.assertEqual(ret, 0)
    # No arguments to show
    boot_show()
    # Clear argument
    boot_clear()
    # No arguments to show
    boot_show()
    # Set argument
    ret, output = self.run_cmd("fim-boot", "set", "bash", "-c", """echo "hello" "world\"""")
    self.assertEqual(output, "")
    self.assertEqual(ret, 0)
    ret, output = self.run_cmd("fim-boot", "show")
    self.assertEqual(ret, 0)
    import json
    boot = json.loads(output)
    self.assertEqual(boot["program"], "bash")
    self.assertEqual(boot["args"], ["-c", """echo "hello" "world\""""])
    # Clear argument
    boot_clear()
    # No arguments to show
    boot_show()