#!/bin/python3

import unittest
import json
import shutil
from pathlib import Path
from .common import BindTestBase
from cli.test_runner import run_cmd

class TestFimBindList(BindTestBase):
  """
  Test suite for fim-bind list command:
  - List all bindings
  - Display binding details (source, destination, type)
  - Validate list command output
  """

  # ===========================================================================
  # fim-bind list Tests
  # ===========================================================================

  def test_list_bindings(self):
    """Test listing all file bindings"""
    test_file_1: Path = self.create_tmp("output1")
    test_file_2: Path = self.create_tmp("output2")
    run_cmd(self.file_image, "fim-bind", "add", "ro", test_file_1, "/host/files/file_1")
    run_cmd(self.file_image, "fim-bind", "add", "ro", test_file_2, "/host/files/file_2")
    out, err, code = run_cmd(self.file_image, "fim-bind", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Check json output
    parsed = json.loads(out)
    self.assertEqual(parsed["0"]["src"], str(test_file_1))
    self.assertEqual(parsed["0"]["dst"], "/host/files/file_1")
    self.assertEqual(parsed["1"]["src"], str(test_file_2))
    self.assertEqual(parsed["1"]["dst"], "/host/files/file_2")
    self.assertEqual(len(parsed), 2)
    # Extra arguments
    out, err, code = run_cmd(self.file_image, "fim-bind", "list", "0")
    self.assertEqual(out, "")
    self.assertIn("'list' command takes no arguments", err)
    self.assertEqual(code, 125)
