#!/bin/python3

from cli.test_base import TestBase

class PermsTestBase(TestBase):
  """
  Base class for permissions tests providing shared utilities
  """

  @classmethod
  def setUpClass(cls):
    super().setUpClass();
    cls.home_custom = cls.dir_data / "user"

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()