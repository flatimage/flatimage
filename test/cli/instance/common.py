#!/bin/python3

import time
from cli.test_base import TestBase

class InstanceTestBase(TestBase):
  """
  Base class for instance tests providing shared utilities
  """
  @classmethod
  def setUpClass(cls):
    super().setUpClass()

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()