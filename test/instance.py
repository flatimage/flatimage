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

  def spawn_cmd(self, *args, env=None):
    return subprocess.Popen(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      stdin=subprocess.PIPE,
      shell=True,
      env=env or os.environ.copy()
    )
    
  def run_cmd(self, *args, env=None):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
    )
    return result.stdout.strip()

  def test_instances(self):
    # Spawn command as root
    self.procs.append(self.spawn_cmd("fim-root", "sleep 100"))
    # Check for the instance value
    output = self.run_cmd("fim-instance", "list")
    self.assertTrue(re.compile("""^(\d+):(\d+)$""").findall(output))
    self.assertEqual(output.count('\n'), 0)
    self.assertEqual(len(re.compile("""(?m)^(\d+):(\d+)$""").findall(output)), 1)
    # Spawn command as regular user
    self.procs.append(self.spawn_cmd("fim-exec", "sleep 100"))
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
