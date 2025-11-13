#!/bin/python3
"""
Base test class for remote tests.
"""

from cli.test_base import TestBase

class RemoteTestBase(TestBase):
  """
  Base class for remote tests. Provides common setup/teardown and utilities for testing fim-remote.
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()