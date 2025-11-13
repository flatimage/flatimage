#!/bin/python3

import shutil
from cli.test_base import TestBase

class BindTestBase(TestBase):
  """
  Base class for binding tests providing shared utilities
  """

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.binding_dir = cls.dir_data / "bind"

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()
    shutil.rmtree(self.binding_dir, ignore_errors=False)

  def create_tmp(self, name):
    """Create a temporary file for binding tests"""
    shutil.rmtree(self.binding_dir, ignore_errors=True)
    self.binding_dir.mkdir(parents=True, exist_ok=False)
    test_file = self.binding_dir / name
    if test_file.exists():
      test_file.unlink()
    return test_file
