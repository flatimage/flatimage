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
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def create_script(self, content):
    """Create a test script in the specified directory"""
    shutil.rmtree(self.dir_image / "data", ignore_errors=True)
    shutil.rmtree(self.dir_image / "tmp", ignore_errors=True)
    hello_script = self.dir_image / "data" / "usr" / "bin" / "hello-world.sh"
    hello_script.parent.mkdir(parents=True, exist_ok=False)
    with open(hello_script, "w+") as f:
      f.write('echo "{}"\n'.format(content))
    os.chmod(hello_script, 0o755)