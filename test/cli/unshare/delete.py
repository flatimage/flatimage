#!/bin/python3

from .common import UnshareTestBase
from cli.test_runner import run_cmd

class TestFimUnshareDel(UnshareTestBase):
  """
  Test suite for fim-unshare del command:
  - Delete individual unshare options
  - Delete multiple unshare options
  - Validate unshare option names
  """

  # ===========================================================================
  # fim-unshare del Tests
  # ===========================================================================

  def test_del_single_option(self):
    """Test deleting a single unshare option"""
    # First add some options
    run_cmd(self.file_image, "fim-unshare", "add", "ipc,pid,uts")

    # Delete one
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del", "pid")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify it was removed
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("ipc", out)
    self.assertIn("uts", out)
    self.assertNotIn("pid", out)
    self.assertEqual(len(lines), 2)

  def test_del_multiple_options(self):
    """Test deleting multiple unshare options at once"""
    # Add options
    run_cmd(self.file_image, "fim-unshare", "add", "user,ipc,pid,net,uts")

    # Delete multiple
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del", "ipc,net,uts")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify they were removed
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("user", out)
    self.assertIn("pid", out)
    self.assertNotIn("ipc", out)
    self.assertNotIn("net", out)
    self.assertNotIn("uts", out)
    self.assertEqual(len(lines), 2)

  def test_del_all_individually(self):
    """Test deleting all options one by one"""
    # Add all options
    run_cmd(self.file_image, "fim-unshare", "set", "all")

    # Delete them one by one
    run_cmd(self.file_image, "fim-unshare", "del", "user")
    run_cmd(self.file_image, "fim-unshare", "del", "ipc")
    run_cmd(self.file_image, "fim-unshare", "del", "pid")
    run_cmd(self.file_image, "fim-unshare", "del", "net")
    run_cmd(self.file_image, "fim-unshare", "del", "uts")
    run_cmd(self.file_image, "fim-unshare", "del", "cgroup")

    # Verify all are gone
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_del_nonexistent_option(self):
    """Test deleting an option that isn't set (should succeed silently)"""
    # Add only ipc
    run_cmd(self.file_image, "fim-unshare", "add", "ipc")

    # Try to delete pid (not set)
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del", "pid")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify ipc is still there
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "ipc")

  def test_del_from_empty(self):
    """Test deleting from empty unshare options (should succeed)"""
    # Ensure empty
    run_cmd(self.file_image, "fim-unshare", "clear")

    # Try to delete
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del", "ipc")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_del_missing_argument(self):
    """Test del command without arguments"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del")
    self.assertEqual(out, "")
    self.assertIn("No arguments for 'DEL' command", err)
    self.assertEqual(code, 125)

  def test_del_invalid_option(self):
    """Test del command with invalid option"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del", "notvalid")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option", err)
    self.assertEqual(code, 125)

  def test_del_invalid_mixed_with_valid(self):
    """Test del command with mix of valid and invalid options"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del", "ipc,notvalid")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option", err)
    self.assertEqual(code, 125)

  def test_del_none_option(self):
    """Test that 'none' cannot be deleted"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del", "none")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option 'NONE'", err)
    self.assertEqual(code, 125)

  def test_del_trailing_arguments(self):
    """Test del command with trailing arguments"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "del", "ipc", "extra")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-unshare: ['extra',]", err)
    self.assertEqual(code, 125)
