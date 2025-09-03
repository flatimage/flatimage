#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib
import json

class TestFimBuildInfo(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def tearDown(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)
    
  def run_cmd(self, *args, env=None):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True,
      env=env or os.environ.copy()
    )
    return result.stdout.strip()

  def test_version(self):
    output = self.run_cmd("fim-version")
    self.assertRegex(output, r"^v\d+\.\d+\.\d+$", msg=f"Invalid version format: {output}")

  def test_version_full(self):
    output = self.run_cmd("fim-version-full")
    try:
      data = json.loads(output)
    except json.JSONDecodeError as e:
      self.fail(f"Invalid JSON output from fim-version-full: {e}\nOutput: {output}")
    # Validate presence of all required fields
    required_fields = {"VERSION", "COMMIT", "DISTRIBUTION", "TIMESTAMP"}
    self.assertEqual(set(data.keys()), required_fields, msg="Missing or extra keys in version-full output")
    # Validate individual fields
    self.assertRegex(data["VERSION"], r"^v\d+\.\d+\.\d+$", msg=f"Invalid VERSION: {data['VERSION']}")
    self.assertRegex(data["COMMIT"], r"^[0-9a-fA-F]{6,}$", msg=f"Invalid COMMIT: {data['COMMIT']}")
    self.assertRegex(data["DISTRIBUTION"], r"^[A-Z]+$", msg=f"Invalid DISTRIBUTION: {data['DISTRIBUTION']}")
    self.assertRegex(data["TIMESTAMP"], r"^\d{14}$", msg=f"Invalid TIMESTAMP: {data['TIMESTAMP']}")