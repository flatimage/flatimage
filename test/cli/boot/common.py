#!/bin/python3

import os
import subprocess
import unittest
import shutil
from cli.test_base import TestBase

class BootTestBase(TestBase):
  """
  Base class for boot configuration tests providing shared utilities
  """

  def setUp(self):
    super().setUp()

  def tearDown(self):
    super().tearDown()