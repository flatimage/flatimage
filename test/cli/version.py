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

  def test_version_short(self):
    re_version = re.compile(r"""v\d+\.\d+\.\d+""")
    out,err,code = self.run_cmd("fim-version", "short")
    self.assertEqual(len(re_version.findall(out)), 1)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_version_full(self):
    re_commit = re.compile("""[A-Za-z0-9]+""")
    re_version = re.compile(r"""v\d+\.\d+\.\d+""")
    out,err,code = self.run_cmd("fim-version", "full")
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

  def test_version_deps(self):
    def check_metadata(json_data, name, licenses, pat_source, pat_version):
      def check_findall(json_data, pattern):
        re_compiled = re.compile(pattern)
        re_matches = re_compiled.findall(json_data)
        self.assertEqual(len(re_matches), 1)
      # Version, Source, SHA
      check_findall(json_data[name]["version"], pat_version)
      check_findall(json_data[name]["source"], pat_source)
      check_findall(json_data[name]["sha"], r"^[0-9a-fA-F]{64}$")
      # Licenses
      for license in licenses:
        license_data = [x for x in json_data[name]["licenses"] if isinstance(x, dict) and x["type"] == license]
        self.assertEqual(len(license_data), 1)
        check_findall(license_data[0]["url"], r"http(?:[s])?://.*")
      # Version
      re_compiled = re.compile(pat_version)
      re_matches = re_compiled.findall(json_data[name]["version"])
      self.assertEqual(len(re_matches), 1)
      # Source
      re_compiled = re.compile(pat_source)
      re_matches = re_compiled.findall(json_data[name]["source"])
      self.assertEqual(len(re_matches), 1)
      # SHA
      re_compiled = re.compile(pat_source)
      re_matches = re_compiled.findall(json_data[name]["source"])
      self.assertEqual(len(re_matches), 1)
    # Get json
    out,err,code = self.run_cmd("fim-version", "deps")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    import json
    json_data = json.loads(out)
    # Validate metadata
    check_metadata(json_data, "bash"
      , ["GPL-3.0"]
      , r"^http(?:[s])?://.*\.tar\.gz$"
      , r"""^\d+(?:\.\d+){2}(?:\(\d+\))?-release$"""
    )
    check_metadata(json_data, "busybox"
      , ["GPLv2"]
      , r"^https://.*busybox.git$"
      , r"""^v\d+(?:\.\d+){2}.git$"""
    )
    check_metadata(json_data, "bwrap"
      , ["LGPLv2+"]
      , r"^https://.*bubblewrap$"
      , r"""^\d+(?:\.\d+){2}$"""
    )
    check_metadata(json_data, "ciopfs"
      , ["GPLv2"]
      , r"^https://.*ciopfs$"
      , r"""^\d+(?:\.\d+){1}$"""
    )
    check_metadata(json_data, "dwarfs_aio"
      , ["GPL-3.0", "MIT"]
      , r"^https://.*dwarfs$"
      , r"""^v\d+(?:\.\d+){2}$"""
    )
    check_metadata(json_data, "overlayfs"
      , ["GPL-2.0"]
      , r"^https://.*fuse-overlayfs$"
      , r"""^\d+(?:\.\d+){1}-dev$"""
    )
    check_metadata(json_data, "unionfs"
      , ["BSD-3-clause"]
      , r"^https://.*unionfs-fuse$"
      , r"""^\d+(?:\.\d+){1}$"""
    )