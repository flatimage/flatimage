#!/bin/python3

import re
import json
from .common import VersionTestBase
from cli.test_runner import run_cmd

class TestFimVersionDeps(VersionTestBase):
  """Test suite for fim-version deps command"""

  def test_version_deps(self):
    """Test dependency metadata output"""
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
    out,err,code = run_cmd(self.file_image, "fim-version", "deps")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
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
