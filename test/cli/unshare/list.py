#!/bin/python3

from .common import UnshareTestBase
from cli.test_runner import run_cmd

class TestFimUnshareList(UnshareTestBase):
  """
  Test suite for fim-unshare list command:
  - List unshare options
  - Validate output format
  """

  # ===========================================================================
  # fim-unshare list Tests
  # ===========================================================================

  def test_list_empty(self):
    """Test listing when no unshare options are set"""
    # Ensure empty
    run_cmd(self.file_image, "fim-unshare", "clear")

    # List
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_list_single_option(self):
    """Test listing a single unshare option"""
    run_cmd(self.file_image, "fim-unshare", "add", "ipc")

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "ipc")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.assertEqual(len(out.strip().splitlines()), 1)

  def test_list_multiple_options(self):
    """Test listing multiple unshare options"""
    run_cmd(self.file_image, "fim-unshare", "add", "user,ipc,pid")

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertEqual(len(lines), 3)
    self.assertIn("user", out)
    self.assertIn("ipc", out)
    self.assertIn("pid", out)

  def test_list_all_options(self):
    """Test listing all unshare options"""
    run_cmd(self.file_image, "fim-unshare", "set", "all")

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertEqual(len(lines), 6)
    self.assertIn("user", out)
    self.assertIn("ipc", out)
    self.assertIn("pid", out)
    self.assertIn("net", out)
    self.assertIn("uts", out)
    self.assertIn("cgroup", out)

  def test_list_output_format(self):
    """Test that list output is one option per line"""
    run_cmd(self.file_image, "fim-unshare", "add", "ipc,pid,uts")

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()

    # Each line should contain exactly one option
    for line in lines:
      self.assertEqual(len(line.strip().split()), 1)

  def test_list_sorted_output(self):
    """Test that list output is sorted alphabetically"""
    run_cmd(self.file_image, "fim-unshare", "add", "uts,ipc,pid")

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()

    # Output should be sorted
    self.assertEqual(lines, sorted(lines))

  def test_list_no_arguments(self):
    """Test that list doesn't accept arguments"""
    run_cmd(self.file_image, "fim-unshare", "add", "ipc")

    # Try to list with arguments (should error)
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list", "extra")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-unshare: ['extra',]", err)
    self.assertEqual(code, 125)

  def test_list_after_add(self):
    """Test listing after adding options"""
    # Add first set
    run_cmd(self.file_image, "fim-unshare", "add", "ipc")
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "ipc")

    # Add more
    run_cmd(self.file_image, "fim-unshare", "add", "pid,uts")
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    lines = out.strip().splitlines()
    self.assertEqual(len(lines), 3)

  def test_list_after_del(self):
    """Test listing after deleting options"""
    # Add options
    run_cmd(self.file_image, "fim-unshare", "add", "user,ipc,pid,net")

    # Delete some
    run_cmd(self.file_image, "fim-unshare", "del", "ipc,net")

    # List remaining
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertEqual(len(lines), 2)
    self.assertIn("user", out)
    self.assertIn("pid", out)
    self.assertNotIn("ipc", out)
    self.assertNotIn("net", out)

  def test_list_after_set(self):
    """Test listing after setting options (replacing)"""
    # Add initial options
    run_cmd(self.file_image, "fim-unshare", "add", "ipc,pid")

    # Set different options
    run_cmd(self.file_image, "fim-unshare", "set", "user,uts")

    # List should show only the new ones
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertEqual(len(lines), 2)
    self.assertIn("user", out)
    self.assertIn("uts", out)
    self.assertNotIn("ipc", out)
    self.assertNotIn("pid", out)

  def test_list_after_clear(self):
    """Test listing after clearing all options"""
    # Add options
    run_cmd(self.file_image, "fim-unshare", "add", "ipc,pid,uts")

    # Clear
    run_cmd(self.file_image, "fim-unshare", "clear")

    # List should be empty
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
