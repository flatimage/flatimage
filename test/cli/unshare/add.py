#!/bin/python3

from .common import UnshareTestBase
from cli.test_runner import run_cmd

class TestFimUnshareAdd(UnshareTestBase):
  """
  Test suite for fim-unshare add command:
  - Add individual unshare options
  - Add multiple unshare options
  - Handle 'all' option
  - Validate unshare option names
  """

  # ===========================================================================
  # fim-unshare add Tests
  # ===========================================================================

  def test_add_single_option(self):
    """Test adding a single unshare option"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "ipc")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "ipc")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_add_multiple_options(self):
    """Test adding multiple unshare options at once"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "ipc,pid,uts")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("ipc", out)
    self.assertIn("pid", out)
    self.assertIn("uts", out)
    self.assertEqual(len(lines), 3)

  def test_add_incremental(self):
    """Test adding unshare options incrementally"""
    # Start with empty
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Add first set
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "ipc,pid")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "ipc\npid")

    # Add more options
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "user,net")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("ipc", out)
    self.assertIn("pid", out)
    self.assertIn("user", out)
    self.assertIn("net", out)
    self.assertEqual(len(lines), 4)

  def test_add_all(self):
    """Test adding all unshare options"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "all")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("user", out)
    self.assertIn("ipc", out)
    self.assertIn("pid", out)
    self.assertIn("net", out)
    self.assertIn("uts", out)
    self.assertIn("cgroup", out)
    self.assertEqual(len(lines), 6)

  def test_add_all_with_others_error(self):
    """Test that 'all' cannot be combined with other options"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "all,ipc")
    self.assertEqual(out, "")
    self.assertIn("Unshare option 'all' should not be used with others", err)
    self.assertEqual(code, 125)

  def test_add_missing_argument(self):
    """Test add command without arguments"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add")
    self.assertEqual(out, "")
    self.assertIn("No arguments for 'ADD' command", err)
    self.assertEqual(code, 125)

  def test_add_invalid_option(self):
    """Test add command with invalid option"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "invalid")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option", err)
    self.assertEqual(code, 125)

  def test_add_invalid_mixed_with_valid(self):
    """Test add command with mix of valid and invalid options"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "ipc,invalid")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option", err)
    self.assertEqual(code, 125)

  def test_add_none_option(self):
    """Test that 'none' cannot be added"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "none")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option 'NONE'", err)
    self.assertEqual(code, 125)

  def test_add_trailing_arguments(self):
    """Test add command with trailing arguments"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "ipc", "extra")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-unshare: ['extra',]", err)
    self.assertEqual(code, 125)

  def test_add_duplicate_option(self):
    """Test adding the same option twice (should be idempotent)"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "ipc")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Add the same option again
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "ipc")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "ipc")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
