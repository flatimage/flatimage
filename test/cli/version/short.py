#!/bin/python3

import re
from .common import VersionTestBase
from cli.test_runner import run_cmd

class TestFimVersionShort(VersionTestBase):
  """Test suite for fim-version short command"""

  def test_version_short(self):
    """Test short version output format"""
    re_version = re.compile(r"""v\d+\.\d+\.\d+|continuous""")
    out,err,code = run_cmd(self.file_image, "fim-version", "short")
    self.assertEqual(len(re_version.findall(out)), 1)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
