#!/bin/python3

from .common import UnshareTestBase
from cli.test_runner import run_cmd

class TestFimUnshareSet(UnshareTestBase):
  """
  Test suite for fim-unshare set command:
  - Set (replace) unshare options
  - Handle 'all' option
  - Validate unshare option names
  """

  # ===========================================================================
  # fim-unshare set Tests
  # ===========================================================================

  def test_set_single_option(self):
    """Test setting a single unshare option"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "pid")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "pid")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_set_multiple_options(self):
    """Test setting multiple unshare options"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "user,ipc,net")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("user", out)
    self.assertIn("ipc", out)
    self.assertIn("net", out)
    self.assertEqual(len(lines), 3)

  def test_set_replaces_existing(self):
    """Test that set replaces all existing options"""
    # Add initial options
    out, err, code = run_cmd(self.file_image, "fim-unshare", "add", "ipc,pid,uts")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify they were added
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertIn("ipc", out)
    self.assertIn("pid", out)
    self.assertIn("uts", out)

    # Now set different options (should replace)
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "user,net")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify old options are gone and new ones are set
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    lines = out.strip().splitlines()
    self.assertIn("user", out)
    self.assertIn("net", out)
    self.assertNotIn("ipc", out)
    self.assertNotIn("pid", out)
    self.assertNotIn("uts", out)
    self.assertEqual(len(lines), 2)

  def test_set_all(self):
    """Test setting all unshare options"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "all")
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

  def test_set_all_with_others_error(self):
    """Test that 'all' cannot be combined with other options"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "all,pid")
    self.assertEqual(out, "")
    self.assertIn("Unshare option 'all' should not be used with others", err)
    self.assertEqual(code, 125)

  def test_set_missing_argument(self):
    """Test set command without arguments"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set")
    self.assertEqual(out, "")
    self.assertIn("No arguments for 'SET' command", err)
    self.assertEqual(code, 125)

  def test_set_invalid_option(self):
    """Test set command with invalid option"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "badoption")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option", err)
    self.assertEqual(code, 125)

  def test_set_invalid_mixed_with_valid(self):
    """Test set command with mix of valid and invalid options"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "ipc,badoption,pid")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option", err)
    self.assertEqual(code, 125)

  def test_set_none_option(self):
    """Test that 'none' cannot be set"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "none")
    self.assertEqual(out, "")
    self.assertIn("Invalid unshare option 'NONE'", err)
    self.assertEqual(code, 125)

  def test_set_trailing_arguments(self):
    """Test set command with trailing arguments"""
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "ipc", "trailing")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-unshare: ['trailing',]", err)
    self.assertEqual(code, 125)

  def test_set_to_empty_previous_all(self):
    """Test setting specific options after having 'all' set"""
    # First set all
    run_cmd(self.file_image, "fim-unshare", "set", "all")

    # Verify all are set
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    lines = out.strip().splitlines()
    self.assertEqual(len(lines), 6)

    # Now set to just one option
    out, err, code = run_cmd(self.file_image, "fim-unshare", "set", "ipc")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Verify only ipc is set
    out, err, code = run_cmd(self.file_image, "fim-unshare", "list")
    self.assertEqual(out.strip(), "ipc")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
