#!/bin/python3

import os
from .common import CasefoldTestBase
from cli.test_runner import run_cmd

class TestFimCasefoldOff(CasefoldTestBase):
  """Test suite for fim-casefold off command"""

  def test_casefold_disabled(self):
    """Test that case-sensitive filesystem works when casefold is disabled"""
    out,err,code = run_cmd(self.file_image, "fim-exec", "mkdir", "-p", "/hElLo/WoRlD")
    self.assertEqual(out, "")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-exec", "stat", "/hello/world")
    self.assertEqual(out, "")
    self.assertIn("No such file or directory", err)
    self.assertEqual(code, 1)

  def test_casefold_off_after_on(self):
    """Test disabling casefold after it was enabled"""
    def success(out,err,code):
      self.assertEqual(out, "")
      self.assertEqual(err, "")
      self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-casefold", "on")
    success(out,err,code)
    out,err,code = run_cmd(self.file_image, "fim-exec", "mkdir", "-p", "/hElLo/WoRlD")
    success(out,err,code)
    out,err,code = run_cmd(self.file_image, "fim-exec", "stat", "/hello/world")
    self.assertIn("File: /hello/world", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-exec", "rmdir", "/hello/world")
    success(out,err,code)
    out,err,code = run_cmd(self.file_image, "fim-casefold", "off")
    self.assertEqual(out, "")
    self.assertIn("casefold cannot be used with bwrap overlayfs, falling back to unionfs", err)
    self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-exec", "mkdir", "-p", "/hElLo/WoRlD")
    success(out,err,code)
    out,err,code = run_cmd(self.file_image, "fim-exec", "stat", "/hElLo/WoRlD")
    self.assertIn("File: /hElLo/WoRlD", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    out,err,code = run_cmd(self.file_image, "fim-exec", "stat", "/hello/world")
    self.assertEqual(out, "")
    self.assertIn("No such file or directory", err)
    self.assertEqual(code, 1)
