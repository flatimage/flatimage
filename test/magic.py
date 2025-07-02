#!/bin/python3

import os
import shutil
import subprocess
import unittest
from pathlib import Path


class TestFimMagic(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]

  def test_magic(self):
    output = subprocess.run(
      ["xxd", "-l", "2", "-s", "8", self.file_image],
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    ).stdout.strip()
    self.assertEqual(output[-2:], "FI")