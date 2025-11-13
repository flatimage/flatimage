#!/bin/python3

import unittest
import json
from cli.test_runner import run_cmd
from .common import DesktopTestBase

class TestFimDesktopSetup(DesktopTestBase):
  """
  Test suite for fim-desktop setup command:
  - Configure desktop integration from JSON file
  - Validate JSON configuration
  - Handle missing or invalid files
  """

  # ===========================================================================
  # fim-desktop setup Tests
  # ===========================================================================

  def test_setup(self):
    """Test setup command with valid and invalid configurations"""
    name = "MyApp"
    self.make_json_setup(r'''"ICON","MIMETYPE","ENTRY"''', name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    desktop = json.loads(out)
    self.assertEqual(desktop["categories"], ["Network", "System"])
    self.assertEqual(desktop["integrations"], ["ENTRY", "MIMETYPE", "ICON"])
    self.assertEqual(desktop["name"], name)
    self.assertEqual(len(desktop), 3)
    # Icon missing
    with open(self.file_desktop, "r") as file:
      desktop = json.loads(file.read())
      desktop["icon"] = "/some/path/to/missing/icon.png"
    with open(self.file_desktop, "w") as file:
      json.dump(desktop, file)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out, "")
    self.assertIn("Could not get size of file '/some/path/to/missing/icon.png': No such file or directory", err)
    self.assertEqual(code, 125)

  def test_setup_cli(self):
    """Test setup command CLI argument validation"""
    # Missing argument
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup")
    self.assertEqual(out, "")
    self.assertIn("Missing argument for 'setup' (/path/to/file.json)", err)
    self.assertEqual(code, 125)
    # Extra argument
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", "some-file.json", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-desktop: ['hello',]", err)
    self.assertEqual(code, 125)
    # Missing json file
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", "some-file.json")
    self.assertEqual(out, "")
    self.assertIn("Failed to open file 'some-file.json' for desktop integration", err)
    self.assertEqual(code, 125)
