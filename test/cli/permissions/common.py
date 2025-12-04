#!/bin/python3

import os
from cli.test_base import TestBase

class PermsTestBase(TestBase):
  """
  Base class for permissions tests providing shared utilities
  """

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.home_custom = cls.dir_data / "user"
    cls.home_orig = os.environ["HOME"]

  def setUp(self):
    super().setUp()

  def tearDown(self):
    os.environ["HOME"] = str(self.home_orig)
    super().tearDown()