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
      stderr=subprocess.STDOUT,
      stdin=subprocess.PIPE,
      env=os.environ.copy(),
      text=True
    )
    return result.stdout.strip()

  def test_instances(self):
    import time
    # Spawn command as root
    self.procs.append(self.spawn_cmd("fim-root", "sleep", "10"))
    time.sleep(1)
    # Check for the instance value
    output = self.run_cmd("fim-instance", "list")
    self.assertEqual(output.count('\n'), 0)
    self.assertEqual(len(re.compile("""(?m)^(\d+):(\d+)$""").findall(output)), 1)
    # Spawn command as regular user
    self.procs.append(self.spawn_cmd("fim-exec", "sleep", "10"))
    time.sleep(1)
    # Check for multiple instances
    output = self.run_cmd("fim-instance", "list")
    pattern = re.compile("""(?m)^(\d+):(\d+)$""")
    self.assertEqual(len(re.compile("""(?m)^(\d+):(\d+)$""").findall(output)), 2)
    self.assertEqual(output.count('\n'), 1)
    # Test root id
    output = self.run_cmd("fim-instance", "exec", "0", "id", "-u")
    self.assertEqual(output, "0")
    # Test regular user id
    output = self.run_cmd("fim-instance", "exec", "1", "id", "-u")
    self.assertEqual(output, str(os.getuid()))
    [proc.kill() for proc in self.procs]
    time.sleep(1)

  def test_cli(self):
    import time
    # Missing arguments for instance
    output = self.run_cmd("fim-instance")
    self.assertTrue(output.endswith("Incorrect number of arguments"))
    # Missing arguments for exec
    output = self.run_cmd("fim-instance", "exec")
    self.assertTrue(output.endswith("Missing id argument"))
    # Invalid id
    output = self.run_cmd("fim-instance", "exec", "foo")
    self.assertTrue(output.endswith("Id argument must be a digit"))
    # Missing instance
    output = self.run_cmd("fim-instance", "exec", "0", "echo", "hello")
    self.assertTrue(output.endswith("No instances are running"))
    # Spawn command as root
    self.procs.append(self.spawn_cmd("fim-exec", "sleep", "10"))
    time.sleep(1)
    # Missing instance command
    output = self.run_cmd("fim-instance", "exec", "0")
    self.assertTrue(output.endswith("Missing command argument"))
    # Out of bounds id
    output = self.run_cmd("fim-instance", "exec", "-1", "echo", "hello")
    self.assertTrue(output.endswith("Id argument must be a digit"))
    output = self.run_cmd("fim-instance", "exec", "1", "echo", "hello")
    self.assertTrue(output.endswith("Instance index out of bounds"))
    [proc.kill() for proc in self.procs]
    time.sleep(1)
