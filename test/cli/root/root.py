#!/bin/python3

from cli.root.common import RootTestBase
from cli.test_runner import run_cmd

class TestFimRoot(RootTestBase):
  """Test fim-root command execution"""

  def test_root_echo(self):
    """Test simple command execution as root"""
    out, err, code = run_cmd(self.file_image, "fim-root", "echo", "test")
    self.assertEqual(out, "test")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_root_uid(self):
    """Test that fim-root executes as UID 0 (root)"""
    out, err, code = run_cmd(self.file_image, "fim-root", "id", "-u")
    self.assertEqual(out, "0")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_root_package_installation(self):
    """Test package installation with package manager"""
    # Get distribution info
    out, err, code = run_cmd(self.file_image, "fim-version", "full")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

    # Set network permission for package installation
    run_cmd(self.file_image, "fim-perms", "set", "network")

    if "ALPINE" in out:
      print("Distribution is alpine")
      out, err, code = run_cmd(self.file_image, "fim-root", "apk", "add", "curl")
      self.assertIn("Installing curl", out)
    elif "ARCH" in out:
      print("Distribution is arch")
      out, err, code = run_cmd(self.file_image, "fim-root", "pacman", "-Sy", "--noconfirm", "curl")
      self.assertIn("Cleaning up downloaded files...", out)
    elif "BLUEPRINT" in out:
      print("Distribution is blueprint")
      # Blueprint has no package manager, skip test
      self.skipTest("Blueprint distribution has no package manager")
    else:
      self.fail("Invalid distribution label")