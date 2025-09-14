#!/bin/python3

import os
import subprocess
import unittest
import shutil
import re
from datetime import date

class TestFimVersion(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def run_cmd(self, *args, env=None):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
      env=env or os.environ.copy()
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_version(self):
    re_version = re.compile("""v\d+\.\d+\.\d+""")
    out,err,code = self.run_cmd("fim-version")
    self.assertEqual(len(re_version.findall(out)), 1)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_version_full(self):
    re_commit = re.compile("""[A-Za-z0-9]+""")
    re_version = re.compile("""v\d+\.\d+\.\d+""")
    out,err,code = self.run_cmd("fim-version-full")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    import json
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