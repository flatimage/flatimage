#!/bin/python3

import os
import subprocess
import unittest
import shutil
from pathlib import Path
from cli.test_base import TestBase

class LayerTestBase(TestBase):
  """
  Base class for layer tests providing shared utilities
  """

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.dir_root = cls.dir_data / "root"

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()

  def create_script(self, dir_root, content):
    """Create a test script in the specified directory"""
    shutil.rmtree(dir_root, ignore_errors=True)
    dir_bin = dir_root / "usr" / "bin"
    dir_bin.mkdir(parents=True, exist_ok=True)
    hello_script = dir_bin / "hello-world.sh"
    with open(hello_script, "w+") as f:
      f.write('echo "{}"\n'.format(content))
    os.chmod(hello_script, 0o755)