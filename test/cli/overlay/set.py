#!/bin/python3

import os
from .common import OverlayTestBase
from cli.test_runner import run_cmd

class TestFimOverlaySet(OverlayTestBase):
  """Test suite for fim-overlay set command"""

  def test_set_show(self):
    """Test setting and verifying overlay types"""
    def success(out,err,code):
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    def check_overlay(name):
      out,err,code = run_cmd(self.file_image, "fim-overlay", "show")
      self.assertEqual(out, name)
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    def check_debug(name):
      os.environ["FIM_DEBUG"] = "1"
      out,_,code = run_cmd(self.file_image, "fim-exec", "echo")
      self.assertIn(f"Overlay type: {name}", out)
      self.assertEqual(code, 0)
      os.environ["FIM_DEBUG"] = "0"
    out,err,code = run_cmd(self.file_image, "fim-overlay", "set", "unionfs")
    success(out,err,code)
    check_overlay("UNIONFS")
    check_debug("UNIONFS_FUSE")
    out,err,code = run_cmd(self.file_image, "fim-overlay", "set", "overlayfs")
    success(out,err,code)
    check_overlay("OVERLAYFS")
    check_debug("FUSE_OVERLAYFS")
    out,err,code = run_cmd(self.file_image, "fim-overlay", "set", "bwrap")
    success(out,err,code)
    check_overlay("BWRAP")
    check_debug("BWRAP")

  def test_set_cli(self):
    """Test CLI argument validation for set command"""
    # Missing arguments
    out,err,code = run_cmd(self.file_image, "fim-overlay", "set")
    self.assertEqual(out, "")
    self.assertIn("Missing argument for 'set'", err)
    self.assertEqual(code, 125)
    # Invalid arguments
    out,err,code = run_cmd(self.file_image, "fim-overlay", "set", "foo")
    self.assertEqual(out, "")
    self.assertIn("Invalid overlay type", err)
    self.assertEqual(code, 125)
    # Trailing arguments
    out,err,code = run_cmd(self.file_image, "fim-overlay", "set", "unionfs", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-overlay: ['foo',]", err)
    self.assertEqual(code, 125)

  def test_set_env(self):
    """Test setting overlay type via environment variable"""
    def check_overlay(name):
      out,err,code = run_cmd(self.file_image, "fim-overlay", "show")
      self.assertEqual(out, name)
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    def check_debug(name):
      os.environ["FIM_DEBUG"] = "1"
      out,_,code = run_cmd(self.file_image, "fim-exec", "echo")
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
