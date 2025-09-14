#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib
import re

class TestFimInstance(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.procs = []
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def tearDown(self):
    for proc in self.procs:
      proc.kill()
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def spawn_cmd(self, *args):
    return subprocess.Popen(
      ["bash", "-c", f"{self.file_image} $@", "--"] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      stdin=subprocess.PIPE,
      env=os.environ.copy()
    )
    
  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      stdin=subprocess.PIPE,
      env=os.environ.copy(),
      text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_instances(self):
    import time
    # Spawn command as root
    self.procs.append(self.spawn_cmd("fim-root", "sleep", "10"))
    time.sleep(1)
    # Extra argument
    out,err,code = self.run_cmd("fim-instance", "list", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-instance: ['foo',]", err)
    self.assertEqual(code, 125)
    # Check for the instance value
    out,err,code = self.run_cmd("fim-instance", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertEqual(out.count('\n'), 0)
    self.assertEqual(len(re.compile("""(?m)^(\d+):(\d+)$""").findall(out)), 1)
    # Spawn command as regular user
    self.procs.append(self.spawn_cmd("fim-exec", "sleep", "10"))
    time.sleep(1)
    # Check for multiple instances
    out,err,code = self.run_cmd("fim-instance", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertEqual(len(re.compile("""(?m)^(\d+):(\d+)$""").findall(out)), 2)
    self.assertEqual(out.count('\n'), 1)
    # Test root id
    out,err,code = self.run_cmd("fim-instance", "exec", "0", "id", "-u")
    self.assertEqual(out, "0")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Test regular user id
    out,err,code = self.run_cmd("fim-instance", "exec", "1", "id", "-u")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertEqual(out, str(os.getuid()))
    [proc.kill() for proc in self.procs]
    time.sleep(1)

  def test_cli(self):
    import time
    # Missing arguments for instance
    out,err,code = self.run_cmd("fim-instance")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-instance' (<exec|list>)", err)
    self.assertEqual(code, 125)
    # Missing arguments for exec
    out,err,code = self.run_cmd("fim-instance", "exec")
    self.assertEqual(out, "")
    self.assertIn("Missing 'id' argument for 'fim-instance'", err)
    self.assertEqual(code, 125)
    # Invalid id
    out,err,code = self.run_cmd("fim-instance", "exec", "foo")
    self.assertEqual(out, "")
    self.assertIn("Id argument must be a digit", err)
    self.assertEqual(code, 125)
    # Missing instance
    out,err,code = self.run_cmd("fim-instance", "exec", "0", "echo", "hello")
    self.assertEqual(out, "")
    self.assertIn("No instances are running", err)
    self.assertEqual(code, 125)
    # Spawn command as root
    self.procs.append(self.spawn_cmd("fim-exec", "sleep", "10"))
    time.sleep(1)
    # Missing instance command
    out,err,code = self.run_cmd("fim-instance", "exec", "0")
    self.assertEqual(out, "")
    self.assertIn("Missing 'cmd' argument for 'fim-instance'", err)
    self.assertEqual(code, 125)
    # Out of bounds id
    out,err,code = self.run_cmd("fim-instance", "exec", "-1", "echo", "hello")
    self.assertEqual(out, "")
    self.assertIn("Id argument must be a digit", err)
    self.assertEqual(code, 125)
    out,err,code = self.run_cmd("fim-instance", "exec", "1", "echo", "hello")
    self.assertEqual(out, "")
    self.assertIn("Instance index out of bounds", err)
    self.assertEqual(code, 125)
    [proc.kill() for proc in self.procs]
    time.sleep(1)
