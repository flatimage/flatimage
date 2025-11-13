#!/bin/python3

import unittest
from pathlib import Path
from .common import BindTestBase
from cli.test_runner import run_cmd

class TestFimBindAdd(BindTestBase):
  """
  Test suite for fim-bind add command:
  - Add read-only bindings
  - Add read-write bindings
  - Add device bindings
  - Validate add command arguments
  """

  # ===========================================================================
  # fim-bind add Tests
  # ===========================================================================

  def test_add_binding(self):
    """Test adding file bindings"""
    # Add binding and read file contents
    test_file: Path = self.create_tmp("output")
    out, err, code = run_cmd(self.file_image, "fim-bind", "add", "ro", test_file, "/host/test_file")
    self.assertIn("Binding index is '0'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    test_file.write_text("binding to inside the container")
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", "/host/test_file")
    self.assertIn("binding to inside the container", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # No option
    out, err, code = run_cmd(self.file_image, "fim-bind", "add")
    self.assertEqual(out, "")
    self.assertIn("Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)", err)
    self.assertEqual(code, 125)
    # Invalid option
    out, err, code = run_cmd(self.file_image, "fim-bind", "add", "roo")
    self.assertEqual(out, "")
    self.assertIn("Invalid bind type", err)
    self.assertEqual(code, 125)
    # No source
    out, err, code = run_cmd(self.file_image, "fim-bind", "add", "ro")
    self.assertEqual(out, "")
    self.assertIn("Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)", err)
    self.assertEqual(code, 125)
    # No dst
    out, err, code = run_cmd(self.file_image, "fim-bind", "add", "ro", "/tmp/test/hello/world")
    self.assertEqual(out, "")
    self.assertIn("Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)", err)
    self.assertEqual(code, 125)
    # Extra args
    out, err, code = run_cmd(self.file_image, "fim-bind", "add", "ro", test_file, "/host/test_file", "heya")
    self.assertEqual(out, "")
    self.assertIn("Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>", err)
    self.assertEqual(code, 125)

  def test_change_file_contents(self):
    """Test modifying bound files (read-write binding)"""
    test_file: Path = self.create_tmp("output")
    test_file.write_text("written by the host")
    # Bind
    run_cmd(self.file_image, "fim-bind", "add", "rw", test_file, "/host/output")
    # Original content
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", "/host/output")
    self.assertEqual(out, "written by the host")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Writeable file
    run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo 'written by the container' > /host/output")
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", "/host/output")
    self.assertEqual(out, "written by the container")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_readonly_file(self):
    """Test read-only binding prevents writes"""
    test_file: Path = self.create_tmp("output")
    test_file.write_text("unchanged")
    run_cmd(self.file_image, "fim-bind", "del", "0")
    run_cmd(self.file_image, "fim-bind", "add", "ro", test_file, "/host/output")
    run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo 'written by the container' > /host/output")
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", "/host/output")
    self.assertEqual(out.strip(), "unchanged")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
