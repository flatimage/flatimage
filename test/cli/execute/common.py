#!/bin/python3

from cli.test_base import TestBase

class ExecTestBase(TestBase):
  """
  Base class for environment tests. Provides common setup/teardown and utilities for testing fim-env.
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()