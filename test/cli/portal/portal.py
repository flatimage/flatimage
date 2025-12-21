#!/bin/python3

import subprocess
from .common import PortalTestBase
from cli.test_runner import run_cmd

class TestFimPortal(PortalTestBase):
  """Test suite for Portal IPC system"""

  def run_host_cmd(self, *args):
    """Run a command directly on the host (not through FlatImage)"""
    result = subprocess.run(
      args,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_portal(self):
    """Test portal command execution from container to host"""
    # FlatImage should not contain Jq
    out,err,code = run_cmd(self.file_image, "fim-exec", "command", "-v", "jq")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 1)
    # Host should contain Jq
    out,err,code = self.run_host_cmd("which", "jq")
    self.assertEqual(out, "/usr/bin/jq")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Execute host Jq through the portal facility
    # -r in jq to avoid double quotes around selected value
    process = subprocess.Popen(
      [self.file_image, "fim-exec", "fim_portal", "jq", "-r", ".key[1]"],
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    # Feed the JSON through stdin and close stdin to signal EOF
    stdout, stderr = process.communicate('{ "key": ["val1", "val2"] }')
    returncode = process.wait()
    self.assertEqual(stdout.strip(), "val2")
    self.assertEqual(stderr.strip(), "")
    self.assertEqual(returncode, 0)

  def test_portal_nonexistent_process(self):
    """Test portal behavior when process name does not exist"""
    # Try to execute a non-existent command through the portal
    _, err, code = run_cmd(
      self.file_image,
      "fim-exec",
      "fim_portal",
      "this_command_definitely_does_not_exist_12345"
    )
    # Error message should indicate the program was not found
    self.assertIn("program not found", err)
    self.assertEqual(code, 1)
