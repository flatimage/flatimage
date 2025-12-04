#!/bin/python3

import re
import json
from datetime import date
from .common import VersionTestBase
from cli.test_runner import run_cmd

class TestFimVersionFull(VersionTestBase):
  """Test suite for fim-version full command"""

  def test_version_full(self):
    """Test full version output with JSON metadata"""
    re_commit = re.compile("""[A-Za-z0-9]+""")
    re_version = re.compile(r"""v\d+\.\d+\.\d+|continuous""")
    out,err,code = run_cmd(self.file_image, "fim-version", "full")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    parsed = json.loads(out)
    self.assertEqual(len(re_commit.findall(parsed["COMMIT"])), 1)
    self.assertIn(parsed["DISTRIBUTION"], ["ALPINE", "ARCH", "BLUEPRINT"])
    timestamp = parsed["TIMESTAMP"]
    self.assertEqual(timestamp[0:4], str(date.today().year))
    self.assertEqual(timestamp[4:6], f"{date.today().month:02d}")
    self.assertEqual(timestamp[6:8], f"{date.today().day:02d}")
    self.assertTrue(0 <= int(timestamp[8:10]) <= 23)
    self.assertTrue(0 <= int(timestamp[10:12]) <= 59)
    self.assertTrue(0 <= int(timestamp[12:14]) <= 59)
    self.assertEqual(len(re_version.findall(parsed["VERSION"])), 1)
