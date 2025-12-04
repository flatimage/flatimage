#!/bin/python3

import unittest
import os
import shutil
from .common import DesktopTestBase
from cli.test_runner import run_cmd

class TestFimDesktopClean(DesktopTestBase):
  """
  Test suite for fim-desktop clean command:
  - Remove desktop integration files
  - Clean MIME types, icons, and desktop entries
  - Handle both PNG and SVG icon formats
  """

  # ===========================================================================
  # fim-desktop clean Tests
  # ===========================================================================

  def test_clean(self):
    """Test cleaning desktop integration files for PNG and SVG icons"""
    def clean(ext_icon):
      # Clean without setup
      out, err, code = run_cmd(self.file_image, "fim-desktop", "clean")
      self.assertEqual(out, "")
      self.assertIn("Empty json data", err)
      self.assertEqual(code, 125)
      # Setup integration
      name = "MyApp"
      path_dir_xdg = self.setup_xdg_data_home()
      self.make_json_setup(r'''"ICON","MIMETYPE","ENTRY"''', name, ext_icon)
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
      out, err, code = run_cmd(self.file_image, "fim-exec", "echo")
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
      # All enabled
      self.check_mime(name, path_dir_xdg, self.assertTrue)
      self.check_mime_generic(name, path_dir_xdg, self.assertTrue)
      self.check_entry(self.file_image, name, path_dir_xdg, self.assertTrue)
      self.check_icons(name, path_dir_xdg, ext_icon, self.assertTrue)
      out, err, code = run_cmd(self.file_image, "fim-desktop", "clean")
      self.assertIn("""xdg_data_home/applications/flatimage-MyApp.desktop'""", out)
      self.assertIn("""xdg_data_home/mime/packages/flatimage-MyApp.xml'""", out)
      self.assertIn("""Updating mime database""", out)
      if ext_icon == 'png':
        self.assertIn("""xdg_data_home/icons/hicolor/16x16/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/16x16/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/22x22/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/22x22/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/24x24/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/24x24/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/32x32/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/32x32/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/48x48/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/48x48/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/64x64/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/64x64/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/96x96/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/96x96/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/128x128/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/128x128/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/256x256/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""xdg_data_home/icons/hicolor/256x256/apps/flatimage_MyApp.png""", out)
      else:
        self.assertIn("""xdg_data_home/icons/hicolor/scalable/apps/flatimage_MyApp.svg""", out)
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
      # All removed
      self.check_mime(name, path_dir_xdg, self.assertFalse)
      self.check_mime_generic(name, path_dir_xdg, self.assertTrue)
      self.check_entry(self.file_image, name, path_dir_xdg, self.assertFalse)
      self.check_icons(name, path_dir_xdg, ext_icon, self.assertFalse)
      # Remove temporary xdg directory
      shutil.rmtree(path_dir_xdg)
      # Reset image
      shutil.copy(os.environ["FILE_IMAGE_SRC"], os.environ["FILE_IMAGE"])
      os.chmod(self.file_image, 0o755)
    # Check for png and svg
    clean('png')
    clean('svg')
