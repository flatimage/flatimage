#!/bin/python3

import os
import subprocess
import unittest
import shutil
import pathlib

class TestFimRemote(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image_src = os.environ["FILE_IMAGE_SRC"]
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def setUp(self):
    shutil.copyfile(self.file_image_src, self.file_image)

  def tearDown(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def run_cmd(self, *args, env=None):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
      env=env or os.environ.copy()
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_remote_cli(self):
    # No arguments to fim-remote
    out,err,code = self.run_cmd("fim-remote")
    self.assertEqual(out, "")
    self.assertIn("Invalid operation for 'fim-remote' (<set|show|clear>)", err)
    self.assertEqual(code, 125)
    # Invalid argument to fim-remote
    out,err,code = self.run_cmd("fim-remote", "sett")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'SETT'", err)
    self.assertEqual(code, 125)

  def test_remote_set(self):
    # No URL argument to set
    out,err,code = self.run_cmd("fim-remote", "set")
    self.assertEqual(out, "")
    self.assertIn("Missing URL for 'set' operation", err)
    self.assertEqual(code, 125)
    # Set a valid URL
    out,err,code = self.run_cmd("fim-remote", "set", "https://example.com/updates")
    self.assertIn("Set remote URL to 'https://example.com/updates'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify it was set
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "https://example.com/updates")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Update to a different URL
    out,err,code = self.run_cmd("fim-remote", "set", "https://mirror.example.org/repo")
    self.assertIn("Set remote URL to 'https://mirror.example.org/repo'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify the update
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "https://mirror.example.org/repo")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Trailing arguments to set
    out,err,code = self.run_cmd("fim-remote", "set", "https://example.com", "extra")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-remote: ['extra',]", err)
    self.assertEqual(code, 125)

  def test_remote_show(self):
    # Show when no URL is set
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "")
    self.assertIn("No remote URL configured", err)
    self.assertEqual(code, 125)
    # Set a URL
    out,err,code = self.run_cmd("fim-remote", "set", "https://cdn.example.net/flatimage")
    self.assertIn("Set remote URL to 'https://cdn.example.net/flatimage'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Show the URL
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "https://cdn.example.net/flatimage")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Trailing arguments to show
    out,err,code = self.run_cmd("fim-remote", "show", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-remote: ['hello',]", err)
    self.assertEqual(code, 125)

  def test_remote_clear(self):
    # Clear when no URL is set
    out,err,code = self.run_cmd("fim-remote", "clear")
    self.assertIn("Cleared remote URL", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify it's still empty
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "")
    self.assertIn("No remote URL configured", err)
    self.assertEqual(code, 125)
    # Set a URL
    out,err,code = self.run_cmd("fim-remote", "set", "https://repo.example.com/images")
    self.assertIn("Set remote URL to 'https://repo.example.com/images'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify it was set
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "https://repo.example.com/images")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Clear the URL
    out,err,code = self.run_cmd("fim-remote", "clear")
    self.assertIn("Cleared remote URL", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify it was cleared
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "")
    self.assertIn("No remote URL configured", err)
    self.assertEqual(code, 125)
    # Trailing arguments to clear
    out,err,code = self.run_cmd("fim-remote", "clear", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-remote: ['hello',]", err)
    self.assertEqual(code, 125)

  def test_remote_workflow(self):
    # Complete workflow: set -> show -> clear -> show
    # Initial state should be empty
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
    # Set a remote URL
    out,err,code = self.run_cmd("fim-remote", "set", "https://download.flatimage.io/stable")
    self.assertEqual(code, 0)
    # Verify it's set
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "https://download.flatimage.io/stable")
    self.assertEqual(code, 0)
    # Clear it
    out,err,code = self.run_cmd("fim-remote", "clear")
    self.assertEqual(code, 0)
    # Verify it's cleared
    out,err,code = self.run_cmd("fim-remote", "show")
    self.assertEqual(out, "")
    self.assertEqual(code, 125)
