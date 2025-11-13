#!/bin/python3

import unittest
from pathlib import Path
from .common import BindTestBase
from cli.test_runner import run_cmd

class TestFimBindDel(BindTestBase):
  """
  Test suite for fim-bind del command:
  - Delete bindings by index
  - Validate index arguments
  - Handle non-existent indices
  """

  # ===========================================================================
  # fim-bind del Tests
  # ===========================================================================

  def test_delete_binding(self):
    """Test deleting file bindings"""
    # Add binding and read file contents
    test_file: Path = self.create_tmp("output")
    out, err, code = run_cmd(self.file_image, "fim-bind", "add", "ro", test_file, "/host/test_file")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertIn("Binding index is '0'", out)
    test_file.write_text("binding to inside the container")
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", "/host/test_file")
    self.assertIn("binding to inside the container", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Check if deletion works
    out, err, code = run_cmd(self.file_image, "fim-bind", "del", "0")
    self.assertIn("Erase element with index '0'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-bind", "list")
    self.assertEqual(out, '')
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", "/host/test_file")
    self.assertEqual(out, '')
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Invalid index
    out, err, code = run_cmd(self.file_image, "fim-bind", "del", "0")
    self.assertIn("No element with index '0' found", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-bind", "del", "aaa")
    self.assertEqual(out, "")
    self.assertIn("Index argument for 'del' is not a number", err)
    self.assertEqual(code, 125)
    # Extra arguments
    out, err, code = run_cmd(self.file_image, "fim-bind", "del", "0", "1")
    self.assertEqual(out, "")
    self.assertIn("Incorrect number of arguments for 'del' (<index>)", err)
    self.assertEqual(code, 125)
