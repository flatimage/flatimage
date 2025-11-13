#!/bin/python3

import unittest
from .common import DesktopTestBase, run_cmd

class TestFimDesktopEnable(DesktopTestBase):
  """
  Test suite for fim-desktop enable command:
  - Enable/disable desktop integration options via CLI
  - Enable/disable options via JSON configuration
  - Validate enable command arguments
  """

  # ===========================================================================
  # fim-desktop enable Tests
  # ===========================================================================

  def test_enable(self):
    """Test enabling desktop integration options via command line"""
    # Setup integration
    name = "MyApp"
    self.make_json_setup("", name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out,
      """{\n"""
      """  "categories": [\n"""
      """    "Network",\n"""
      """    "System"\n"""
      """  ],\n"""
      """  "integrations": [],\n"""
      """  "name": "MyApp"\n"""
      """}"""
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Nothing enabled
    self.check_enabled_options(name, self.assertFalse, self.assertFalse, self.assertFalse)
    # Mimetype
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable", "mimetype")
    self.assertEqual(out, "MIMETYPE")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertTrue, self.assertFalse, self.assertFalse)
    # Icon
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable", "icon")
    self.assertEqual(out, "ICON")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertFalse, self.assertTrue, self.assertFalse)
    # Desktop entry
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable", "entry")
    self.assertEqual(out, "ENTRY")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertFalse, self.assertFalse, self.assertTrue)
    # All
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable", "entry,mimetype,icon")
    self.assertEqual(out, "ENTRY\nMIMETYPE\nICON")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertTrue, self.assertTrue, self.assertTrue)
    # None
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable", "none")
    self.assertEqual(out, "NONE")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertFalse, self.assertFalse, self.assertFalse)

  def test_enable_json(self):
    """Test enabling desktop integration options via JSON configuration"""
    name = "MyApp"
    # Mimetype
    self.make_json_setup('''"MIMETYPE"''', name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out,
      """{\n"""
      """  "categories": [\n"""
      """    "Network",\n"""
      """    "System"\n"""
      """  ],\n"""
      """  "integrations": [\n"""
      """    "MIMETYPE"\n"""
      """  ],\n"""
      """  "name": "MyApp"\n"""
      """}"""
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertTrue, self.assertFalse, self.assertFalse)
    # Icons
    self.make_json_setup('''"ICON"''', name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out,
      """{\n"""
      """  "categories": [\n"""
      """    "Network",\n"""
      """    "System"\n"""
      """  ],\n"""
      """  "integrations": [\n"""
      """    "ICON"\n"""
      """  ],\n"""
      """  "name": "MyApp"\n"""
      """}"""
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertFalse, self.assertTrue, self.assertFalse)
    # Desktop entry
    self.make_json_setup('''"ENTRY"''', name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out,
      """{\n"""
      """  "categories": [\n"""
      """    "Network",\n"""
      """    "System"\n"""
      """  ],\n"""
      """  "integrations": [\n"""
      """    "ENTRY"\n"""
      """  ],\n"""
      """  "name": "MyApp"\n"""
      """}"""
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertFalse, self.assertFalse, self.assertTrue)
    # All
    self.make_json_setup('''"ENTRY","MIMETYPE","ICON"''', name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out,
      """{\n"""
      """  "categories": [\n"""
      """    "Network",\n"""
      """    "System"\n"""
      """  ],\n"""
      """  "integrations": [\n"""
      """    "ENTRY",\n"""
      """    "MIMETYPE",\n"""
      """    "ICON"\n"""
      """  ],\n"""
      """  "name": "MyApp"\n"""
      """}"""
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertTrue, self.assertTrue, self.assertTrue)
    # Invalid
    self.make_json_setup('''"ICONN"''', name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out, "")
    self.assertIn("Failed to deserialize json", err)
    self.assertEqual(code, 125)

  def test_enable_cli(self):
    """Test enable command CLI argument validation"""
    # Missing arguments
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable")
    self.assertEqual(out, "")
    self.assertIn("Missing arguments for 'enable' (entry,mimetype,icon,none)", err)
    self.assertEqual(code, 125)
    # Extra arguments
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable", "icon", "mimetype")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-desktop: ['mimetype',]", err)
    self.assertEqual(code, 125)
    # none + others
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable", "none,mimetype")
    self.assertEqual(out, "")
    self.assertIn("'none' option should not be used with others", err)
    self.assertEqual(code, 125)
    # Invalid arguments
    out, err, code = run_cmd(self.file_image, "fim-desktop", "enable", "icon2")
    self.assertEqual(out, "")
    self.assertIn("Invalid integration item", err)
    self.assertEqual(code, 125)

  # ===========================================================================
  # Desktop Integration Path and State Tests
  # ===========================================================================

  def test_path_change(self):
    """Test that desktop integration updates when binary path changes"""
    import os
    import shutil
    from pathlib import Path
    
    name = "MyApp"
    path_dir_xdg = self.setup_xdg_data_home()
    # Setup integration
    self.make_json_setup(r'''"ENTRY","MIMETYPE","ICON"''', name)
    os.environ["FIM_DEBUG"] = "1"
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertIn("""{\n"""
      """  "categories": [\n"""
      """    "Network",\n"""
      """    "System"\n"""
      """  ],\n"""
      """  "integrations": [\n"""
      """    "ENTRY",\n"""
      """    "MIMETYPE",\n"""
      """    "ICON"\n"""
      """  ],\n"""
      """  "name": "MyApp"\n"""
      """}"""
      , out
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # First run integrates mime database
    out, err, code = run_cmd(self.file_image, "fim-exec", "echo")
    self.assertIn("Updating mime database", out)
    self.assertNotIn("Updating mime database", err)
    self.assertEqual(code, 0)
    # Second run detects it is already integrated
    out, err, code = run_cmd(self.file_image, "fim-exec", "echo")
    self.assertIn("Skipping mime database update", out)
    self.assertNotIn("Skipping mime database update", err)
    self.assertEqual(code, 0)
    # Check if entry has correct binary path
    self.check_entry(self.file_image, name, path_dir_xdg, self.assertTrue)
    # Copy file to another path
    file_image = Path(self.file_image).parent / "temp.flatimage"
    shutil.copyfile(self.file_image, file_image)
    os.chmod(file_image, 0o755)
    # Run again
    out, err, code = run_cmd(str(file_image), "fim-exec", "echo")
    self.assertIn("Updating mime database", out)
    self.assertNotIn("Updating mime database", err)
    self.assertEqual(code, 0)
    self.check_entry(file_image, name, path_dir_xdg, self.assertTrue)
    out, err, code = run_cmd(str(file_image), "fim-exec", "echo")
    self.assertIn("Skipping mime database update", out)
    self.assertNotIn("Skipping mime database update", err)
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"] = "0"

  # ===========================================================================
  # Edge Cases and Error Handling
  # ===========================================================================

  def test_name_with_spaces(self):
    """Test desktop integration with application name containing spaces"""
    name = "My App"
    _ = self.setup_xdg_data_home()
    # Setup integration
    self.make_json_setup(r'''"ENTRY","MIMETYPE","ICON"''', name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out,
      """{\n"""
      """  "categories": [\n"""
      """    "Network",\n"""
      """    "System"\n"""
      """  ],\n"""
      """  "integrations": [\n"""
      """    "ENTRY",\n"""
      """    "MIMETYPE",\n"""
      """    "ICON"\n"""
      """  ],\n"""
      """  "name": "My App"\n"""
      """}"""
    )
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-exec", "echo")
    # Check for the correct files paths
    self.check_enabled_options(name, self.assertTrue, self.assertTrue, self.assertTrue)

  def test_name_with_slash(self):
    """Test desktop integration with invalid application name containing slash"""
    name = "My/App"
    _ = self.setup_xdg_data_home()
    # Setup integration
    self.make_json_setup(r'''"ENTRY","MIMETYPE","ICON"''', name)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out, "")
    self.assertIn("Application name cannot contain the '/' character", err)
    self.assertEqual(code, 125)
