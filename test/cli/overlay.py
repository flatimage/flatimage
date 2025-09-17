#!/bin/python3

import os
import subprocess
import unittest
from pathlib import Path


class TestFimOverlay(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_script = Path(__file__).resolve().parent

  def setUp(self):
    os.environ["FIM_DEBUG"] = "0"
    os.environ["FIM_OVERLAY"] = ""

  def tearDown(self):
    os.environ["FIM_DEBUG"] = "0"
    os.environ["FIM_OVERLAY"] = ""

  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def test_default(self):
    out,err,code = self.run_cmd("fim-overlay", "show")
    self.assertEqual(out, "BWRAP")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_set_show(self):
    def success(out,err,code):
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    def check_overlay(name):
      out,err,code = self.run_cmd("fim-overlay", "show")
      self.assertEqual(out, name)
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    def check_debug(name):
      os.environ["FIM_DEBUG"] = "1"
      out,_,code = self.run_cmd("fim-exec", "echo")
      self.assertIn(f"Overlay type: {name}", out)
      self.assertEqual(code, 0)
      os.environ["FIM_DEBUG"] = "0"
    out,err,code = self.run_cmd("fim-overlay", "set", "unionfs")
    success(out,err,code)
    check_overlay("UNIONFS")
    check_debug("UNIONFS_FUSE")
    out,err,code = self.run_cmd("fim-overlay", "set", "overlayfs")
    success(out,err,code)
    check_overlay("OVERLAYFS")
    check_debug("FUSE_OVERLAYFS")
    out,err,code = self.run_cmd("fim-overlay", "set", "bwrap")
    success(out,err,code)
    check_overlay("BWRAP")
    check_debug("BWRAP")
    # Missing arguments
    out,err,code = self.run_cmd("fim-overlay", "set")
    self.assertEqual(out, "")
    self.assertIn("Missing argument for 'set'", err)
    self.assertEqual(code, 125)
    # Invalid arguments
    out,err,code = self.run_cmd("fim-overlay", "set", "foo")
    self.assertEqual(out, "")
    self.assertIn("Could not determine enum entry from 'FOO'", err)
    self.assertEqual(code, 125)
    # Trailing arguments
    out,err,code = self.run_cmd("fim-overlay", "show", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-overlay: ['foo',]", err)
    self.assertEqual(code, 125)
    out,err,code = self.run_cmd("fim-overlay", "set", "unionfs", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-overlay: ['foo',]", err)
    self.assertEqual(code, 125)

  def test_set_env(self):
    def check_overlay(name):
      out,err,code = self.run_cmd("fim-overlay", "show")
      self.assertEqual(out, name)
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    def check_debug(name):
      os.environ["FIM_DEBUG"] = "1"
      out,_,code = self.run_cmd("fim-exec", "echo")
      self.assertIn(f"Overlay type: {name}", out)
      self.assertEqual(code, 0)
      os.environ["FIM_DEBUG"] = "0"
    os.environ["FIM_OVERLAY"] = "unionfs"
    check_overlay("UNIONFS")
    check_debug("UNIONFS_FUSE")
    os.environ["FIM_OVERLAY"] = "overlayfs"
    check_overlay("OVERLAYFS")
    check_debug("FUSE_OVERLAYFS")
    os.environ["FIM_OVERLAY"] = "bwrap"
    check_overlay("BWRAP")
    check_debug("BWRAP")
    # Invalid value
    os.environ["FIM_OVERLAY"] = "hello"
    check_overlay("BWRAP")
    check_debug("BWRAP")
    # Empty variable
    os.environ["FIM_OVERLAY"] = ""
    check_overlay("BWRAP")
    check_debug("BWRAP")