#!/bin/python3

from cli.test_base import TestBase

class VersionTestBase(TestBase):
  """
  Base class for version tests providing shared utilities
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()