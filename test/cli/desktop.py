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

  def setUp(self):
    shutil.copy(os.environ["FILE_IMAGE_SRC"], os.environ["FILE_IMAGE"])

  def tearDown(self):
    os.environ["FIM_DEBUG"] = "0"
    os.environ["HOME"] = self.home_host
    shutil.rmtree(self.dir_script / "home_tmp", ignore_errors=True)
    if self.file_desktop.exists():
      os.unlink(self.file_desktop)
    image_file = Path(self.file_image).parent / "temp.flatimage"
    if image_file.exists():
      os.unlink(image_file)
    image_dir = Path(self.file_image).parent / ".temp.flatimage.config"
    if image_dir.exists():
      shutil.rmtree(image_dir)
    if self.dir_image.exists():
      shutil.rmtree(self.dir_image)
    file_png = self.dir_script / "out.png"
    if file_png.exists():
      os.unlink(file_png)
    file_svg = self.dir_script / "out.svg"
    if file_svg.exists():
      os.unlink(file_svg)


  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def run_cmd2(self, image, *args):
    result = subprocess.run(
      [image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  
  def make_json_setup(self, integrations, name, ext_icon='png'):
    with open(self.file_desktop, "w") as file:
      file.write(
      """{""" "\n"
      rf"""  "integrations": [{integrations}],""" "\n"
      rf"""  "name": "{name}",""" "\n"
      rf"""  "icon": "{self.dir_script}/icon.{ext_icon}",""" "\n"
      """  "categories": ["System", "Network"]""" "\n"
      """}""" "\n"
    )

  def check_mime_generic(self, name, path_dir_xdg, fun):
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
    if ext == 'png':
      for i in [16,22,24,32,48,64,96,128,256]:
        fun((path_dir_xdg / "icons" / "hicolor" / f"{i}x{i}" / "apps" / f"flatimage_{name}.png").exists())
    else:
      fun((path_dir_xdg / "icons" / "hicolor" / "scalable" / "apps" / f"flatimage_{name}.svg").exists())

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
        """Categories=Network;System;"""
      )
      with open(path_dir_entry, "r") as file:
        contents = file.read()
        self.assertEqual(expected, contents)

  def test_setup(self):
    name = "MyApp"
    self.make_json_setup(r'''"ICON","MIMETYPE","ENTRY"''', name)
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    import json
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
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out, "")
    self.assertIn("Could not get size of file '/some/path/to/missing/icon.png': No such file or directory", err)
    self.assertEqual(code, 125)

  def test_setup_cli(self):
    # Missing argument
    out,err,code = self.run_cmd("fim-desktop", "setup")
    self.assertEqual(out, "")
    self.assertIn("Missing argument from 'setup' (/path/to/file.json)", err)
    self.assertEqual(code, 125)
    # Extra argument
    out,err,code = self.run_cmd("fim-desktop", "setup", "some-file.json", "hello")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-desktop: ['hello',]", err)
    self.assertEqual(code, 125)
    # Missing json file
    out,err,code = self.run_cmd("fim-desktop", "setup", "some-file.json")
    self.assertEqual(out, "")
    self.assertIn("Failed to open file 'some-file.json' for desktop integration", err)
    self.assertEqual(code, 125)

  def setup_alt_home(self):
    path_dir_home = self.dir_script / "home_tmp"
    path_dir_xdg = path_dir_home / ".local" / "share"
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=False)
    os.environ["HOME"] = str(path_dir_home)
    return (path_dir_xdg, path_dir_home)

  def check_enabled_options(self, name, fchk1 ,fchk2, fchk3):
    path_dir_xdg, path_dir_home = self.setup_alt_home()
    out,err,code = self.run_cmd("fim-exec", "echo")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_mime(name, path_dir_xdg, fchk1)
    self.check_mime_generic(name, path_dir_xdg, fchk1)
    self.check_icons(name, path_dir_xdg, "png", fchk2)
    self.check_entry(self.file_image, name, path_dir_xdg, fchk3)
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=False)
 
  def test_enable(self):
    # Setup integration
    name = "MyApp"
    self.make_json_setup("", name)
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
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
    out,err,code = self.run_cmd("fim-desktop", "enable", "mimetype")
    self.assertEqual(out, "MIMETYPE")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertTrue, self.assertFalse, self.assertFalse)
    # Icon
    out,err,code = self.run_cmd("fim-desktop", "enable", "icon")
    self.assertEqual(out, "ICON")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertFalse, self.assertTrue, self.assertFalse)
    # Desktop entry
    out,err,code = self.run_cmd("fim-desktop", "enable", "entry")
    self.assertEqual(out, "ENTRY")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertFalse, self.assertFalse, self.assertTrue)
    # All
    out,err,code = self.run_cmd("fim-desktop", "enable", "entry,mimetype,icon")
    self.assertEqual(out, "ENTRY\nMIMETYPE\nICON")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertTrue, self.assertTrue, self.assertTrue)
    # None
    out,err,code = self.run_cmd("fim-desktop", "enable", "none")
    self.assertEqual(out, "NONE")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    self.check_enabled_options(name, self.assertFalse, self.assertFalse, self.assertFalse)

  def test_enable_json(self):
    name = "MyApp"
    # Mimetype
    self.make_json_setup('''"MIMETYPE"''', name)
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
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
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
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
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
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
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
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
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out, "")
    self.assertIn("Failed to deserialize json: Could not determine enum entry from 'ICONN'", err)
    self.assertEqual(code, 125)

  def test_enable_cli(self):
    # Missing arguments
    out,err,code = self.run_cmd("fim-desktop", "enable")
    self.assertEqual(out, "")
    self.assertIn("Missing arguments for 'enable' (desktop,entry,mimetype,none)", err)
    self.assertEqual(code, 125)
    # Extra arguments
    out,err,code = self.run_cmd("fim-desktop", "enable", "icon", "mimetype")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-desktop: ['mimetype',]", err)
    self.assertEqual(code, 125)
    # none + others
    out,err,code = self.run_cmd("fim-desktop", "enable", "none,mimetype")
    self.assertEqual(out, "")
    self.assertIn("'none' option should not be used with others", err)
    self.assertEqual(code, 125)
    # Invalid arguments
    out,err,code = self.run_cmd("fim-desktop", "enable", "icon2")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'ICON2'", err)
    self.assertEqual(code, 125)

  def test_path_change(self):
    name = "MyApp"
    path_dir_xdg, _ = self.setup_alt_home()
    # Setup integration
    self.make_json_setup(r'''"ENTRY","MIMETYPE","ICON"''', name)
    os.environ["FIM_DEBUG"] = "1"
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
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
    out,err,code = self.run_cmd("fim-exec", "echo")
    self.assertIn("Updating mime database...", out)
    self.assertNotIn("Updating mime database...", err)
    self.assertEqual(code, 0)
    # Second run detects it is already integrated
    out,err,code = self.run_cmd("fim-exec", "echo")
    self.assertIn("Skipping mime database update...", out)
    self.assertNotIn("Skipping mime database update...", err)
    self.assertEqual(code, 0)
    # Check if entry has correct binary path
    self.check_entry(self.file_image, name, path_dir_xdg, self.assertTrue)
    # Copy file to another path
    file_image = Path(self.file_image).parent / "temp.flatimage"
    shutil.copyfile(self.file_image, file_image)
    os.chmod(file_image, 0o755)
    # Run again
    out,err,code = self.run_cmd2(file_image, "fim-exec", "echo")
    self.assertIn("Updating mime database...", out)
    self.assertNotIn("Updating mime database...", err)
    self.assertEqual(code, 0)
    self.check_entry(file_image, name, path_dir_xdg, self.assertTrue)
    out,err,code = self.run_cmd2(file_image, "fim-exec", "echo")
    self.assertIn("Skipping mime database update...", out)
    self.assertNotIn("Skipping mime database update...", err)
    self.assertEqual(code, 0)

  def test_name_with_spaces(self):
    name = "My App"
    _ = self.setup_alt_home()
    # Setup integration
    self.make_json_setup(r'''"ENTRY","MIMETYPE","ICON"''', name)
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
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
    out,err,code = self.run_cmd("fim-exec", "echo")
    # Check for the correct files paths
    self.check_enabled_options(name, self.assertTrue, self.assertTrue, self.assertTrue)

  def test_name_with_slash(self):
    name = "My/App"
    _ = self.setup_alt_home()
    # Setup integration
    self.make_json_setup(r'''"ENTRY","MIMETYPE","ICON"''', name)
    out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.assertEqual(out, "")
    self.assertIn("Application name cannot contain the '/' character", err)
    self.assertEqual(code, 125)

  def test_clean(self):
    def clean(ext_icon):
      # Clean without setup
      out,err,code = self.run_cmd("fim-desktop", "clean")
      self.assertEqual(out, "")
      self.assertIn("Empty json data", err)
      self.assertEqual(code, 125)
      # Setup integration
      name = "MyApp"
      path_dir_xdg, _ = self.setup_alt_home()
      self.make_json_setup(r'''"ICON","MIMETYPE","ENTRY"''', name, ext_icon)
      out,err,code = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
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
      out,err,code = self.run_cmd("fim-exec", "echo")
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
      # All enabled
      self.check_mime(name, path_dir_xdg, self.assertTrue)
      self.check_mime_generic(name, path_dir_xdg, self.assertTrue)
      self.check_entry(self.file_image, name, path_dir_xdg, self.assertTrue)
      self.check_icons(name, path_dir_xdg, ext_icon, self.assertTrue)
      out,err,code = self.run_cmd("fim-desktop", "clean")
      self.assertIn("""home_tmp/.local/share/applications/flatimage-MyApp.desktop'""", out)
      self.assertIn("""home_tmp/.local/share/mime/packages/flatimage-MyApp.xml'""", out)
      self.assertIn("""Updating mime database...""", out)
      if ext_icon == 'png':
        self.assertIn("""home_tmp/.local/share/icons/hicolor/16x16/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/16x16/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/22x22/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/22x22/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/24x24/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/24x24/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/32x32/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/32x32/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/48x48/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/48x48/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/64x64/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/64x64/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/96x96/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/96x96/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/128x128/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/128x128/apps/flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/256x256/mimetypes/application-flatimage_MyApp.png'""", out)
        self.assertIn("""home_tmp/.local/share/icons/hicolor/256x256/apps/flatimage_MyApp.png""", out)
      else:
        self.assertIn("""home_tmp/.local/share/icons/hicolor/scalable/apps/flatimage_MyApp.svg""", out)
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
    # Check for png and svg
    clean('png')
    clean('svg')

  def test_dump_nosetup(self):
    self.setup_alt_home()
    out,err,code = self.run_cmd("fim-desktop", "dump", "entry")
    self.assertEqual(out, "")
    self.assertIn("Empty json data", err)
    self.assertEqual(code, 125)
    out,err,code = self.run_cmd("fim-desktop", "dump", "mimetype")
    self.assertEqual(out, "")
    self.assertIn("Empty json data", err)
    self.assertEqual(code, 125)
    out,err,code = self.run_cmd("fim-desktop", "dump", "icon", self.dir_script / "out.png")
    self.assertEqual(out, "")
    self.assertIn("Empty icon data", err)
    self.assertEqual(code, 125)

  def test_dump_icon(self):
    def dump_icon(ext_icon):
      name = "MyApp"
      # Setup desktop integration
      self.make_json_setup(r'''"ICON"''', name, ext_icon)
      self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
      # Dump icon
      self.setup_alt_home()
      file_icon = self.dir_script / f"out.{ext_icon}"
      out,err,code = self.run_cmd("fim-desktop", "dump", "icon", str(file_icon))
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
      self.assertTrue(file_icon.exists())
      # Compare icon SHA with source
      sha_base = subprocess.run(
        ["sha256sum", str(self.dir_script / f"icon.{ext_icon}")],
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
      out,err,code = self.run_cmd("fim-desktop", "dump", "icon", str(self.dir_script / "out"))
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
      out,err,code = self.run_cmd("fim-desktop", "clean")
      self.assertIn("Removed file", out)
      self.assertEqual(code, 0)
      out,err,code = self.run_cmd("fim-desktop", "dump", "icon", file_icon)
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    # Test for PNG and SVG
    dump_icon('png')
    dump_icon('svg')

  def test_dump_entry(self):
    name = "MyApp"
    self.setup_alt_home()
    # Setup desktop integration
    self.make_json_setup(r'''"ENTRY"''', name)
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    # Dump desktop entry
    out,err,code = self.run_cmd("fim-desktop", "dump", "entry")
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
    out,err,code = self.run_cmd("fim-desktop", "clean")
    self.assertIn("Removed file", out)
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-desktop", "dump", "entry")
    self.assertEqual(out, expected)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_dump_mimetype(self):
    self.maxDiff = None
    name = "MyApp"
    # Setup desktop integration
    self.make_json_setup(r'''"MIMETYPE"''', name)
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    # Dump mime type
    self.setup_alt_home()
    out,err,code = self.run_cmd("fim-desktop", "dump", "mimetype")
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
    out,err,code = self.run_cmd("fim-desktop", "clean")
    self.assertIn("Removed file", out)
    self.assertEqual(code, 0)
    out,err,code = self.run_cmd("fim-desktop", "dump", "mimetype")
    self.assertEqual(out, expected)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)