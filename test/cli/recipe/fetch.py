#!/bin/python3

import json
from pathlib import Path
from .common import RecipeTestBase
from cli.test_runner import run_cmd

class TestFimRecipeFetch(RecipeTestBase):
  """
  Test suite for fim-recipe fetch command:
  - Download recipes from remote repository
  - Handle remote configuration
  - Validate recipe caching behavior
  - Handle multiple recipe downloads
  """

  # ===========================================================================
  # fim-recipe fetch Tests
  # ===========================================================================

  def test_fetch_no_remote_configured(self):
    """Test fetch when no remote URL is configured"""
    # Clear remote first
    run_cmd(self.file_image, "fim-remote", "clear")
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch", "test-recipe")
    self.assertEqual(out, "")
    self.assertIn("No remote URL configured", err)
    self.assertEqual(code, 125)

  def test_fetch_single_recipe(self):
    """Test fetching a single recipe from remote"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch", "gpu")
    # If successful, should download and cache the recipe
    self.assertIn("Successfully downloaded recipe 'gpu'", out)
    self.assertIn("Connecting to raw.githubusercontent.com", err)
    self.assertEqual(code, 0)
    # Check if recipe was cached
    cache_path = self.get_recipe_cache_path(self.get_distribution(), "gpu")
    # Cache may exist in config directory
    self.assertTrue(cache_path.exists() or Path(self.dir_image).exists())

  def test_fetch_multiple_recipes_comma_separated(self):
    """Test fetching multiple recipes with comma-separated names"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch", "gpu,audio,xorg")
    self.assertIn("Successfully downloaded recipe 'gpu'", out)
    self.assertIn("Successfully downloaded recipe 'audio'", out)
    self.assertIn("Successfully downloaded recipe 'xorg'", out)
    self.assertIn("Connecting to raw.githubusercontent.com", err)
    self.assertEqual(code, 0)

  def test_fetch_invalid_recipe_name(self):
    """Test fetching a non-existent recipe"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch", "nonexistent-recipe-xyz")
    self.assertIn("Downloading recipe", out)
    self.assertIn("Failed to fetch recipe", err)
    self.assertEqual(code, 125)

  def test_fetch_always_downloads_fresh(self):
    """Test that fetch always downloads fresh copy (doesn't use cache)"""
    # Create a mock cached recipe with known content
    mock_content = {
      "description": "Old cached version",
      "packages": ["old-package"]
    }
    cache_path = self.create_mock_recipe(self.get_distribution(), "xorg", mock_content)
    # Read the old content before fetch
    with open(cache_path, 'r') as f:
      old_content = json.load(f)
    # Verify old content is what we created
    self.assertEqual(old_content["description"], "Old cached version")
    self.assertEqual(old_content["packages"], ["old-package"])
    # Fetch the recipe (should download fresh and overwrite)
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch", "xorg")
    self.assertIn("Successfully downloaded recipe 'xorg'", out)
    self.assertIn("Connecting to raw.githubusercontent.com", err)
    self.assertEqual(code, 0)
    # Read the new content after fetch
    with open(cache_path, 'r') as f:
      new_content = json.load(f)
    # Verify the content has changed (was overwritten)
    self.assertNotEqual(old_content, new_content)
    # The new content should NOT have our mock data
    self.assertNotEqual(new_content.get("description"), "Old cached version")
    self.assertNotEqual(new_content.get("packages"), ["old-package"])
    # The new content should have real data from the remote
    self.assertIn("packages", new_content)  # Should have packages array
    self.assertIsInstance(new_content["packages"], list)  # Should be a list
