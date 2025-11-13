#!/bin/python3

import unittest
import os
import subprocess
from pathlib import Path
from .common import DesktopTestBase
from cli.test_runner import run_cmd

class TestFimDesktopDump(DesktopTestBase):
  """
  Test suite for fim-desktop dump command:
  - Export desktop entry data
  - Export MIME type data
  - Export icon data (PNG and SVG)
  - Handle missing setup data
  """

  # ===========================================================================
  # fim-desktop dump Tests
  # ===========================================================================

  def test_dump_nosetup(self):
    """Test dump command when desktop integration is not set up"""
    self.setup_xdg_data_home()
    out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "entry")
    self.assertEqual(out, "")
    self.assertIn("Empty json data", err)
    self.assertEqual(code, 125)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "mimetype")
    self.assertEqual(out, "")
    self.assertIn("Empty json data", err)
    self.assertEqual(code, 125)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "icon", self.dir_data / "out.png")
    self.assertEqual(out, "")
    self.assertIn("Empty icon data", err)
    self.assertEqual(code, 125)

  def test_dump_icon(self):
    """Test dumping icon data to PNG and SVG files"""
    def dump_icon(ext_icon):
      name = "MyApp"
      # Setup desktop integration
      self.make_json_setup(r'''"ICON"''', name, ext_icon)
      run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
      # Dump icon
      self.setup_xdg_data_home()
      file_icon = self.dir_data / f"out.{ext_icon}"
      out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "icon", str(file_icon))
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
      self.assertTrue(file_icon.exists())
      # Compare icon SHA with source
      sha_base = subprocess.run(
        ["sha256sum", str(self.dir_data / f"icon.{ext_icon}")],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
      )
      sha_target = subprocess.run(
        ["sha256sum", str(file_icon)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
      )
      self.assertEqual(sha_base.stdout[:64], sha_target.stdout[:64])
      # Generate extension
      os.remove(file_icon)
      out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "icon", str(self.dir_data / "out"))
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
      sha_target = subprocess.run(
        ["sha256sum", file_icon],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
      )
      self.assertEqual(sha_base.stdout[:64], sha_target.stdout[:64])
      # Test clean
      out, err, code = run_cmd(self.file_image, "fim-desktop", "clean")
      self.assertIn("Removed file", out)
      self.assertEqual(code, 0)
      out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "icon", file_icon)
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    # Test for PNG and SVG
    dump_icon('png')
    dump_icon('svg')

  def test_dump_entry(self):
    """Test dumping desktop entry data"""
    name = "MyApp"
    self.setup_xdg_data_home()
    # Setup desktop integration
    self.make_json_setup(r'''"ENTRY"''', name)
    run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    # Dump desktop entry
    out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "entry")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Check if entry matches expected output
    expected: str = (
      """[Desktop Entry]""" "\n"
      rf"""Name={name}""" "\n"
      """Type=Application""" "\n"
      rf'''Comment=FlatImage distribution of "{name}"''' "\n"
      rf"""Exec="{self.file_image}" %F""" "\n"
      rf"""Icon=flatimage_{name}""" "\n"
      rf"""MimeType=application/flatimage_{name};""" "\n"
      """Categories=Network;System;"""
    )
    self.assertEqual(expected, out)
    # Test clean
    out, err, code = run_cmd(self.file_image, "fim-desktop", "clean")
    self.assertIn("Removed file", out)
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "entry")
    self.assertEqual(out, expected)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_dump_mimetype(self):
    """Test dumping MIME type data"""
    self.maxDiff = None
    name = "MyApp"
    # Setup desktop integration
    self.make_json_setup(r'''"MIMETYPE"''', name)
    run_cmd(self.file_image, "fim-desktop", "setup", str(self.file_desktop))
    # Dump mime type
    self.setup_xdg_data_home()
    out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "mimetype")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Check if mime type matches expected output
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
    self.assertEqual(expected, out)
    # Test clean
    out, err, code = run_cmd(self.file_image, "fim-desktop", "clean")
    self.assertIn("Removed file", out)
    self.assertEqual(code, 0)
    out, err, code = run_cmd(self.file_image, "fim-desktop", "dump", "mimetype")
    self.assertEqual(out, expected)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
