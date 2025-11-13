#!/bin/python3

import unittest
from cli.test_base import TestBase

class CasefoldTestBase(TestBase):
  """
  Base class for casefold tests providing shared utilities
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()