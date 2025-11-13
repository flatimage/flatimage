#!/bin/python3

import os
from .common import CasefoldTestBase
from cli.test_runner import run_cmd

class TestFimCasefoldOn(CasefoldTestBase):
  """Test suite for fim-casefold on command"""

  def test_casefold_enabled(self):
    """Test that case-insensitive filesystem works when enabled"""
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
    out,err,code = run_cmd(self.file_image, "fim-exec", "stat", "/hEllO/WorlD")
    self.assertIn("File: /hEllO/WorlD", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"]="1"
    out,err,code = run_cmd(self.file_image, "fim-exec", "echo", "-n", "")
    self.assertIn("Overlay type: UNIONFS", out)
    self.assertEqual([l for l in out.splitlines() if not l.startswith("D::") and not l.startswith("I::") and not len(l) == 0] , [])
    self.assertIn("casefold cannot be used with bwrap overlayfs, falling back to unionfs", err)
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"]="0"
    out,err,code = run_cmd(self.file_image, "fim-exec", "rmdir", "/hello/world")
    success(out,err,code)
