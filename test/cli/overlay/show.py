#!/bin/python3

import os
from .common import OverlayTestBase
from cli.test_runner import run_cmd

class TestFimOverlayShow(OverlayTestBase):
  """Test suite for fim-overlay show command"""

  def test_default(self):
    """Test default overlay type"""
    out,err,code = run_cmd(self.file_image, "fim-overlay", "show")
    self.assertEqual(out, "BWRAP")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_show_trailing_args(self):
    """Test error handling for trailing arguments"""
    out,err,code = run_cmd(self.file_image, "fim-overlay", "show", "foo")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments for fim-overlay: ['foo',]", err)
    self.assertEqual(code, 125)
