#!/bin/python3
"""
Base test class for environment tests.
"""

import unittest
from cli.test_base import TestBase

class EnvTestBase(TestBase):
  """
  Base class for environment tests. Provides common setup/teardown and utilities for testing fim-env.
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()