#!/bin/python3

import os
import shutil
import subprocess
import unittest
import json
from pathlib import Path

class TestFimDesktop(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = Path(os.environ["DIR_IMAGE"])
    cls.dir_script = Path(__file__).resolve().parent
    cls.file_desktop = cls.dir_script / "desktop.json"
    cls.home_host = os.environ["HOME"]

  def tearDown(self):
    os.environ["HOME"] = self.home_host
    shutil.rmtree(self.dir_script / "home1", ignore_errors=True)
    shutil.rmtree(self.file_desktop, ignore_errors=True)

  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    )
    return result.stdout.strip()

  def check_mime(self, name, path_dir_xdg, fun):
    fun((path_dir_xdg / "mime" / "packages" / "flatimage.xml").exists())
    fun((path_dir_xdg / "mime" / "packages" / f"flatimage-{name}.xml").exists())

  def check_icons(self, name, path_dir_xdg, ext, fun):
    for i in [16,22,24,32,48,64,96,128,256]:
      fun((path_dir_xdg / "icons" / "hicolor" / f"{i}x{i}" / "apps" / f"application-flatimage_{name}.{ext}").exists())

  def check_entry(self, name, path_dir_xdg, fun):
    fun((path_dir_xdg / "applications" / f"flatimage-{name}.desktop").exists())

  def test_setup(self):
    with open(self.file_desktop, "w+") as file:
      file.write(f"""
      {{
        "integrations": [],
        "name": "MyCoolApp",
        "icon": "{self.dir_script}/icon.png",
        "categories": ["System"]
      }}
    """)
    output = self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    import json
    desktop = json.loads(output)
    self.assertEqual(desktop["categories"], ["System"])
    self.assertEqual(desktop["integrations"], [])
    self.assertEqual(desktop["name"], "MyCoolApp")
    self.assertEqual(len(desktop), 3)

  def test_enable(self):
    path_dir_home = self.dir_script / "home1"
    path_dir_xdg = path_dir_home / ".local" / "share"
    os.environ["HOME"] = str(path_dir_home)
    with open(self.file_desktop, "w+") as file:
      file.write(f"""
      {{
        "integrations": [],
        "name": "MyCoolApp",
        "icon": "{self.dir_script}/icon.png",
        "categories": ["System"]
      }}
    """)
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=True)
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    self.run_cmd("fim-exec", "echo")
    self.check_mime("MyCoolApp", path_dir_xdg, self.assertFalse)
    self.check_icons("MyCoolApp", path_dir_xdg, "png", self.assertFalse)
    self.check_entry("MyCoolApp", path_dir_xdg, self.assertFalse)
    # Mimetype
    self.run_cmd("fim-desktop", "enable", "mimetype")
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=True)
    self.run_cmd("fim-exec", "echo")
    self.check_mime("MyCoolApp", path_dir_xdg, self.assertTrue)
    self.check_icons("MyCoolApp", path_dir_xdg, "png", self.assertFalse)
    self.check_entry("MyCoolApp", path_dir_xdg, self.assertFalse)
    # Icon
    self.run_cmd("fim-desktop", "enable", "icon")
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=True)
    self.run_cmd("fim-exec", "echo")
    self.check_mime("MyCoolApp", path_dir_xdg, self.assertFalse)
    self.check_icons("MyCoolApp", path_dir_xdg, "png", self.assertTrue)
    self.check_entry("MyCoolApp", path_dir_xdg, self.assertFalse)
    # Desktop entry
    self.run_cmd("fim-desktop", "enable", "entry")
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=True)
    self.run_cmd("fim-exec", "echo")
    self.check_mime("MyCoolApp", path_dir_xdg, self.assertFalse)
    self.check_icons("MyCoolApp", path_dir_xdg, "png", self.assertFalse)
    self.check_entry("MyCoolApp", path_dir_xdg, self.assertTrue)
    # From json
    ### Mimetype
    with open(self.file_desktop, "w+") as file:
      file.write(f"""
      {{
        "integrations": ["MIMETYPE"],
        "name": "MyCoolApp",
        "icon": "{self.dir_script}/icon.png",
        "categories": ["System"]
      }}
    """)
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=True)
    self.run_cmd("fim-exec", "echo")
    self.check_mime("MyCoolApp", path_dir_xdg, self.assertTrue)
    self.check_icons("MyCoolApp", path_dir_xdg, "png", self.assertFalse)
    self.check_entry("MyCoolApp", path_dir_xdg, self.assertFalse)
    ## Icons
    with open(self.file_desktop, "w+") as file:
      file.write(f"""
      {{
        "integrations": ["ICON"],
        "name": "MyCoolApp",
        "icon": "{self.dir_script}/icon.png",
        "categories": ["System"]
      }}
    """)
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=True)
    self.run_cmd("fim-exec", "echo")
    self.check_mime("MyCoolApp", path_dir_xdg, self.assertFalse)
    self.check_icons("MyCoolApp", path_dir_xdg, "png", self.assertTrue)
    self.check_entry("MyCoolApp", path_dir_xdg, self.assertFalse)
    ## Desktop entry
    with open(self.file_desktop, "w+") as file:
      file.write(f"""
      {{
        "integrations": ["ENTRY"],
        "name": "MyCoolApp",
        "icon": "{self.dir_script}/icon.png",
        "categories": ["System"]
      }}
    """)
    self.run_cmd("fim-desktop", "setup", str(self.file_desktop))
    shutil.rmtree(path_dir_home, ignore_errors=True)
    path_dir_home.mkdir(parents=True, exist_ok=True)
    self.run_cmd("fim-exec", "echo")
    self.check_mime("MyCoolApp", path_dir_xdg, self.assertFalse)
    self.check_icons("MyCoolApp", path_dir_xdg, "png", self.assertFalse)
    self.check_entry("MyCoolApp", path_dir_xdg, self.assertTrue)