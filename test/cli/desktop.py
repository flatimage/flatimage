#!/bin/python3

import os
import shutil
import subprocess
import unittest
import json
from pathlib import Path
import shutil

class TestFimDesktop(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = Path(os.environ["DIR_IMAGE"])
    cls.dir_script = Path(__file__).resolve().parent
    cls.file_desktop = cls.dir_script / "desktop.json"
    cls.home_host = os.environ["HOME"]

  def tearDown(self):
    os.environ["FIM_DEBUG"] = "0"
    os.environ["HOME"] = self.home_host
    shutil.rmtree(self.dir_script / "home_tmp", ignore_errors=True)
    if Path.exists(self.file_desktop):
      os.unlink(self.file_desktop)
    image_file = Path(self.file_image).parent / "temp.flatimage"
    if Path.exists(image_file):
      os.unlink(image_file)
    image_dir = Path(self.file_image).parent / ".temp.flatimage.config"
    if Path.exists(image_dir):
      shutil.rmtree(image_dir)


  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    )
    return result.stdout.strip()

  def run_cmd2(self, image, *args):
    result = subprocess.run(
      [image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    )
    return result.stdout.strip()

  
  def make_json_setup(self, integrations):
    with open(self.file_desktop, "w") as file:
      file.write(
      """{""" "\n"
      rf"""  "integrations": [{integrations}],""" "\n"
      """  "name": "MyCoolApp",""" "\n"
      rf"""  "icon": "{self.dir_script}/icon.png",""" "\n"
      """  "categories": ["System", "Network"]""" "\n"
      """}""" "\n"
    )

  def check_mime(self, name, path_dir_xdg, fun):
    self.maxDiff = None
    path_dir_mime_generic = path_dir_xdg / "mime" / "packages" / "flatimage.xml"
    path_dir_mime = path_dir_xdg / "mime" / "packages" / f"flatimage-{name}.xml"
    fun(path_dir_mime_generic.exists())
    fun(path_dir_mime.exists())
    if path_dir_mime.exists():
      expected = (
        r"""<?xml version="1.0" encoding="UTF-8"?>""" "\n"
        r"""<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">""" "\n"
        rf"""  <mime-type type="application/flatimage_{name}">""" "\n"
        r"""    <comment>FlatImage Application</comment>""" "\n"
        rf"""    <glob weight="100" pattern="{Path(self.file_image).name}"/>""" "\n"
        r"""    <sub-class-of type="application/x-executable"/>""" "\n"
        r"""    <generic-icon name="application-flatimage"/>""" "\n"
        r"""  </mime-type>""" "\n"
        r"""</mime-info>""" "\n"
      )
      with open(path_dir_mime, 'r') as file:
        contents = file.read()
        self.assertEqual(expected, contents)
    if path_dir_mime_generic.exists():
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
      with open(path_dir_mime_generic, 'r') as file:
        contents = file.read()
        self.assertEqual(expected, contents)



  def check_icons(self, name, path_dir_xdg, ext, fun):
    for i in [16,22,24,32,48,64,96,128,256]:
      fun((path_dir_xdg / "icons" / "hicolor" / f"{i}x{i}" / "apps" / f"flatimage_{name}.{ext}").exists())

  def check_entry(self, image, name, path_dir_xdg, fun):
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
        """Categories=Network;System;""" "\n"
      )
      with open(path_dir_entry, "r") as file:
        contents = file.read()
        self.assertEqual(expected, contents)

  def test_setup(self):
    self.make_json_setup(r'''"ICON","MIMETYPE","ENTRY"''')
    output = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    import json
    desktop = json.loads(output)
    self.assertEqual(desktop["categories"], ["Network", "System"])
    self.assertEqual(desktop["integrations"], ["ENTRY", "MIMETYPE", "ICON"])
    self.assertEqual(desktop["name"], "MyCoolApp")
    self.assertEqual(len(desktop), 3)
    # Icon missing
    with open(self.file_desktop, "r") as file:
      desktop = json.loads(file.read())
      desktop["icon"] = "/some/path/to/missing/icon.png"
    with open(self.file_desktop, "w") as file:
      json.dump(desktop, file)
    output = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.assertIn("Could not get size of file '/some/path/to/missing/icon.png': No such file or directory", output)

  def test_setup_cli(self):
    # Missing argument
    output = self.run_cmd("fim-desktop", "setup")
    self.assertIn("Missing argument from 'setup' (/path/to/file.json)", output)
    # Extra argument
    output = self.run_cmd("fim-desktop", "setup", "some-file.json", "hello")
    self.assertIn("Trailing arguments for fim-desktop: ['hello',]", output)
    # Missing json file
    output = self.run_cmd("fim-desktop", "setup", "some-file.json")
    self.assertIn("Failed to open file 'some-file.json' for desktop integration", output)

  def setup_alt_home(self):
    name = "MyCoolApp"
    path_dir_home = self.dir_script / "home_tmp"
    path_dir_xdg = path_dir_home / ".local" / "share"
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=False)
    os.environ["HOME"] = str(path_dir_home)
    return (name, path_dir_xdg, path_dir_home)

  def check_enabled_options(self, fchk1 ,fchk2, fchk3):
    name, path_dir_xdg, path_dir_home = self.setup_alt_home()
    self.run_cmd("fim-exec", "echo")
    self.check_mime(name, path_dir_xdg, fchk1)
    self.check_icons(name, path_dir_xdg, "png", fchk2)
    self.check_entry(self.file_image, name, path_dir_xdg, fchk3)
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=False)
 
  def test_enable(self):
    # Setup integration
    self.make_json_setup("")
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    # Nothing enabled
    self.check_enabled_options(self.assertFalse, self.assertFalse, self.assertFalse)
    # Mimetype
    self.run_cmd("fim-desktop", "enable", "mimetype")
    self.check_enabled_options(self.assertTrue, self.assertFalse, self.assertFalse)
    # Icon
    self.run_cmd("fim-desktop", "enable", "icon")
    self.check_enabled_options(self.assertFalse, self.assertTrue, self.assertFalse)
    # Desktop entry
    self.run_cmd("fim-desktop", "enable", "entry")
    self.check_enabled_options(self.assertFalse, self.assertFalse, self.assertTrue)
    # All
    self.run_cmd("fim-desktop", "enable", "entry,mimetype,icon")
    self.check_enabled_options(self.assertTrue, self.assertTrue, self.assertTrue)
    # None
    self.run_cmd("fim-desktop", "enable", "none")
    self.check_enabled_options(self.assertFalse, self.assertFalse, self.assertFalse)

  def test_enable_json(self):
    # Mimetype
    self.make_json_setup('''"MIMETYPE"''')
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.check_enabled_options(self.assertTrue, self.assertFalse, self.assertFalse)
    # Icons
    self.make_json_setup('''"ICON"''')
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.check_enabled_options(self.assertFalse, self.assertTrue, self.assertFalse)
    # Desktop entry
    self.make_json_setup('''"ENTRY"''')
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.check_enabled_options(self.assertFalse, self.assertFalse, self.assertTrue)
    # All
    self.make_json_setup('''"ENTRY","MIMETYPE","ICON"''')
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.check_enabled_options(self.assertTrue, self.assertTrue, self.assertTrue)
    # Invalid
    self.make_json_setup('''"ICONN"''')
    output = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.assertIn("Failed to deserialize json: Could not determine enum entry from 'ICONN'", output)

  def test_enable_cli(self):
    # Missing arguments
    output = self.run_cmd("fim-desktop", "enable")
    self.assertIn("Missing arguments for 'enable' (desktop,entry,mimetype,none)", output)
    # Extra arguments
    output = self.run_cmd("fim-desktop", "enable", "icon", "mimetype")
    self.assertIn("Trailing arguments for fim-desktop: ['mimetype',]", output)
    # none + others
    output = self.run_cmd("fim-desktop", "enable", "none,mimetype")
    self.assertIn("'none' option should not be used with others", output)
    # Invalid arguments
    output = self.run_cmd("fim-desktop", "enable", "icon2")
    self.assertIn("Could not determine enum entry from 'ICON2'", output)

  def test_path_change(self):
    name, path_dir_xdg, _ = self.setup_alt_home()
    # Setup integration
    self.make_json_setup(r'''"ENTRY","MIMETYPE","ICON"''')
    os.environ["FIM_DEBUG"] = "1"
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    # First run integrates mime database
    output = self.run_cmd("fim-exec", "echo")
    self.assertIn("Updating mime database...", output)
    # Second run detects it is already integrated
    output = self.run_cmd("fim-exec", "echo")
    self.assertIn("Skipping mime database update...", output)
    # Check if entry has correct binary path
    self.check_entry(self.file_image, name, path_dir_xdg, self.assertTrue)
    # Copy file to another path
    file_image = Path(self.file_image).parent / "temp.flatimage"
    shutil.copyfile(self.file_image, file_image)
    os.chmod(file_image, 0o755)
    # Run again
    output = self.run_cmd2(file_image, "fim-exec", "echo")
    self.assertIn("Updating mime database...", output)
    self.check_entry(file_image, name, path_dir_xdg, self.assertTrue)
    output = self.run_cmd2(file_image, "fim-exec", "echo")
    self.assertIn("Skipping mime database update...", output)