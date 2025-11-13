#!/bin/python3

import os
import shutil
import subprocess
import unittest
from pathlib import Path
from cli.test_runner import run_cmd
from cli.test_base import TestBase

class DesktopTestBase(TestBase):
  """
  Base class for desktop integration tests providing shared utilities
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()

  def make_json_setup(self, integrations, name, ext_icon='png'):
    """Create a desktop integration JSON configuration file"""
    with open(self.file_desktop, "w") as file:
      file.write(
      """{""" "\n"
      rf"""  "integrations": [{integrations}],""" "\n"
      rf"""  "name": "{name}",""" "\n"
      rf"""  "icon": "{self.dir_data}/icon.{ext_icon}",""" "\n"
      """  "categories": ["System", "Network"]""" "\n"
      """}""" "\n"
    )

  def check_mime_generic(self, name, path_dir_xdg, fun):
    """Verify the generic FlatImage MIME type registration file"""
    path_file_mime_generic = path_dir_xdg / "mime" / "packages" / "flatimage.xml"
    fun(path_file_mime_generic.exists())
    if path_file_mime_generic.exists():
      expected = (
        """<?xml version="1.0" encoding="UTF-8"?>""" "\n"
        """<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">""" "\n"
        """  <mime-type type="application/flatimage">""" "\n"
        """    <comment>FlatImage Application</comment>""" "\n"
        """    <magic>""" "\n"
        """      <match value="ELF" type="string" offset="1">""" "\n"
        """        <match value="0x46" type="byte" offset="8">""" "\n"
        """          <match value="0x49" type="byte" offset="9">""" "\n"
        """            <match value="0x01" type="byte" offset="10"/>""" "\n"
        """          </match>""" "\n"
        """        </match>""" "\n"
        """      </match>""" "\n"
        """    </magic>""" "\n"
        rf"""    <glob weight="50" pattern="*.flatimage"/>""" "\n"
        """    <sub-class-of type="application/x-executable"/>""" "\n"
        """    <generic-icon name="application-flatimage"/>""" "\n"
        """  </mime-type>""" "\n"
        """</mime-info>""" "\n"
      )
      with open(path_file_mime_generic, 'r') as file:
        contents = file.read()
        self.assertEqual(expected, contents)

  def check_mime(self, name, path_dir_xdg, fun):
    """Verify the application-specific MIME type registration file"""
    path_file_mime = path_dir_xdg / "mime" / "packages" / f"flatimage-{name}.xml"
    fun(path_file_mime.exists())
    if path_file_mime.exists():
      expected = (
        r"""<?xml version="1.0" encoding="UTF-8"?>""" "\n"
        r"""<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">""" "\n"
        rf"""  <mime-type type="application/flatimage_{name}">""" "\n"
        r"""    <comment>FlatImage Application</comment>""" "\n"
        rf"""    <glob weight="100" pattern="{Path(self.file_image).name}"/>""" "\n"
        r"""    <sub-class-of type="application/x-executable"/>""" "\n"
        r"""    <generic-icon name="application-flatimage"/>""" "\n"
        r"""  </mime-type>""" "\n"
        r"""</mime-info>"""
      )
      with open(path_file_mime, 'r') as file:
        contents = file.read()
        self.assertEqual(expected, contents)

  def check_icons(self, name, path_dir_xdg, ext, fun):
    """Verify icon files are installed in appropriate directories"""
    if ext == 'png':
      for i in [16,22,24,32,48,64,96,128,256]:
        fun((path_dir_xdg / "icons" / "hicolor" / f"{i}x{i}" / "apps" / f"flatimage_{name}.png").exists())
    else:
      fun((path_dir_xdg / "icons" / "hicolor" / "scalable" / "apps" / f"flatimage_{name}.svg").exists())

  def check_entry(self, image, name, path_dir_xdg, fun):
    """Verify the desktop entry file exists and has correct content"""
    path_dir_entry = path_dir_xdg / "applications" / f"flatimage-{name}.desktop"
    # Check if it exists or not
    fun(path_dir_entry.exists())
    # If it does exist, also check the file contents
    if path_dir_entry.exists():
      expected: str = (
        """[Desktop Entry]""" "\n"
        rf"""Name={name}""" "\n"
        """Type=Application""" "\n"
        rf'''Comment=FlatImage distribution of "{name}"''' "\n"
        rf"""Exec="{image}" %F""" "\n"
        rf"""Icon=flatimage_{name}""" "\n"
        rf"""MimeType=application/flatimage_{name};""" "\n"
        """Categories=Network;System;"""
      )
      with open(path_dir_entry, "r") as file:
        contents = file.read()
        self.assertEqual(expected, contents)

  def setup_xdg_data_home(self):
    """Set up temporary XDG_DATA_HOME directory for testing"""
    path_dir_xdg = self.dir_xdg
    shutil.rmtree(path_dir_xdg, ignore_errors=True)
    path_dir_xdg.mkdir(parents=True, exist_ok=False)
    os.environ["XDG_DATA_HOME"] = str(path_dir_xdg)
    return path_dir_xdg

  def check_enabled_options(self, name, fchk1, fchk2, fchk3):
    """Verify which desktop integration options are enabled"""
    path_dir_xdg = self.setup_xdg_data_home()
    out, err, code = run_cmd(self.file_image, "fim-exec", "echo")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_mime(name, path_dir_xdg, fchk1)
    self.check_mime_generic(name, path_dir_xdg, fchk1)
    self.check_icons(name, path_dir_xdg, "png", fchk2)
    self.check_entry(self.file_image, name, path_dir_xdg, fchk3)
    shutil.rmtree(path_dir_xdg, ignore_errors=True)
    path_dir_xdg.mkdir(parents=True, exist_ok=False)
