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

  def create_script(self, content, dir_root=None):
    """Create a test script in the image directory"""
    if not dir_root:
      dir_root = self.dir_image / "data"
    shutil.rmtree(dir_root, ignore_errors=True)
    hello_script = dir_root / "usr" / "bin" / "hello-world.sh"
    hello_script.parent.mkdir(parents=True, exist_ok=False)
    with open(hello_script, "w+") as f:
      f.write('echo "{}"\n'.format(content))
    os.chmod(hello_script, 0o755)