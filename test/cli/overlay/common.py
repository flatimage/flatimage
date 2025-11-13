#!/bin/python3

from pathlib import Path
from cli.test_base import TestBase

class OverlayTestBase(TestBase):
  """
  Base class for overlay tests providing shared utilities
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()