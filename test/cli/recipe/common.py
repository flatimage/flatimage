#!/bin/python3

import json
import shutil
from pathlib import Path
from cli.test_base import TestBase
from cli.test_runner import run_cmd

class RecipeTestBase(TestBase):
  """
  Base class for recipe tests providing shared utilities
  """

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.remote_url = "https://raw.githubusercontent.com/flatimage/recipes/master"

  def setUp(self):
    super().setUp()
    # Set up remote URL for recipe tests
    run_cmd(self.file_image, "fim-remote", "set", self.remote_url)
    run_cmd(self.file_image, "fim-perms", "add", "network")

  def get_distribution(self):
    """Get the distribution name from the image"""
    out, _, _ = run_cmd(self.file_image, "fim-version", "full")
    data = json.loads(out)
    return data["DISTRIBUTION"].lower()

  def get_config_dir(self):
    """Get the configuration directory for recipe cache"""
    return Path(self.dir_image)

  def get_recipe_cache_path(self, distro, recipe_name):
    """Construct expected path for cached recipe file"""
    return self.get_config_dir() / "recipes" / distro / "latest" / f"{recipe_name}.json"

  def create_mock_recipe(self, distro, recipe_name, content):
    """Create a mock recipe file in the cache directory"""
    cache_path = self.get_recipe_cache_path(distro, recipe_name)
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    with open(cache_path, 'w') as f:
      json.dump(content, f)
    return cache_path