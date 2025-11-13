#!/bin/python3

from cli.test_base import TestBase

class PortalTestBase(TestBase):
  """
  Base class for portal tests providing shared utilities
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()