#!/bin/python3

from .common import UnshareTestBase
from cli.test_runner import run_cmd

class TestFimUnshareClear(UnshareTestBase):
  """
  Test suite for fim-unshare clear command:
  - Clear all unshare options
  - Validate behavior
  """

  # ===========================================================================
  # fim-unshare clear Tests
  # ===========================================================================

  def test_clear_with_options_set(self):
    """Test clearing when unshare options are set"""
    # Add some options
    run_cmd(self.file_image, "fim-unshare", "add", "user,ipc,pid,net")

    # Verify they were added
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertNotEqual(out, "")

    # Clear
    out, err, code = run_cmd(self.file_image, "fim-unshare", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify all cleared
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_clear_when_already_empty(self):
    """Test clearing when no unshare options are set"""
    # Ensure empty
    run_cmd(self.file_image, "fim-unshare", "clear")

    # Verify empty
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out, "")

    # Clear again (should succeed)
    out, err, code = run_cmd(self.file_image, "fim-unshare", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_clear_all_options(self):
    """Test clearing after setting all options"""
    # Set all options
    run_cmd(self.file_image, "fim-unshare", "set", "all")

    # Verify all are set
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    lines = out.strip().splitlines()
    self.assertEqual(len(lines), 6)

    # Clear
    out, err, code = run_cmd(self.file_image, "fim-unshare", "clear")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify all cleared
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_clear_no_arguments(self):
    """Test that clear doesn't accept arguments"""
    # Add some options
    run_cmd(self.file_image, "fim-unshare", "add", "ipc")

    # Try to clear with arguments (should error)
    out, err, code = run_cmd(self.file_image, "fim-unshare", "clear", "extra")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-unshare: ['extra',]", err)
    self.assertEqual(code, 125)

    # Verify options weren't cleared
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "ipc")

  def test_clear_idempotent(self):
    """Test that clearing multiple times is idempotent"""
    # Add options
    run_cmd(self.file_image, "fim-unshare", "add", "ipc,pid")

    # Clear once
    out, err, code = run_cmd(self.file_image, "fim-unshare", "clear")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Clear again
    out, err, code = run_cmd(self.file_image, "fim-unshare", "clear")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Clear third time
    out, err, code = run_cmd(self.file_image, "fim-unshare", "clear")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify still empty
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out, "")
