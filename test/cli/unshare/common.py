#!/bin/python3

from cli.test_base import TestBase
from cli.test_runner import run_cmd

class UnshareTestBase(TestBase):
  """
  Base class for unshare tests providing shared utilities
  """

  @classmethod
  def setUpClass(cls):
    super().setUpClass()

  def setUp(self):
    super().setUp()

  def tearDown(self):
    # Clear unshare options after each test
    run_cmd(self.file_image, "fim-unshare", "clear")
    super().tearDown()
