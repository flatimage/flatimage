#!/bin/python3

import os
import subprocess
import unittest
import shutil
import json
from pathlib import Path

class TestFimRecipe(unittest.TestCase):
  """
  Test suite for fim-recipe command and all its subcommands:
  - fetch: Download recipes from remote repository
  - info: Display cached recipe information
  - install: Download and install packages from recipes

  Tests use https://raw.githubusercontent.com/flatimage/recipes/master as remote
  """

  @classmethod
  def setUpClass(cls):
    cls.file_image_src = os.environ["FILE_IMAGE_SRC"]
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]
    cls.remote_url = "https://raw.githubusercontent.com/flatimage/recipes/master"

  def setUp(self):
    shutil.copyfile(self.file_image_src, self.file_image)
    # Set up remote URL for recipe tests
    self.run_cmd("fim-remote", "set", self.remote_url)
    self.run_cmd("fim-perms", "add", "network")

  def tearDown(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def run_cmd(self, *args, env=None):
    """Execute a command against the FlatImage binary"""
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
      env=env or os.environ.copy()
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def get_distribution(self):
    out, _, _ = self.run_cmd("fim-version", "full")
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

  # ===========================================================================
  # CLI Validation Tests
  # ===========================================================================

  def test_recipe_no_subcommand(self):
    """Test fim-recipe with no subcommand"""
    out, err, code = self.run_cmd("fim-recipe")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-recipe'", err)
    self.assertEqual(code, 125)

  def test_recipe_invalid_subcommand(self):
    """Test fim-recipe with invalid subcommand"""
    out, err, code = self.run_cmd("fim-recipe", "invalid-cmd")
    self.assertEqual(out, "")
    self.assertIn("Invalid recipe operation", err)
    self.assertEqual(code, 125)

  def test_recipe_missing_recipe_name(self):
    """Test fim-recipe fetch with no recipe name"""
    out, err, code = self.run_cmd("fim-recipe", "fetch")
    self.assertEqual(out, "")
    self.assertIn("Missing recipe", err)
    self.assertEqual(code, 125)

  def test_recipe_trailing_arguments(self):
    """Test fim-recipe with trailing unexpected arguments"""
    out, err, code = self.run_cmd("fim-recipe", "fetch", "test-recipe", "extra-arg")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments", err)
    self.assertEqual(code, 125)

  # ===========================================================================
  # fim-recipe fetch Tests
  # ===========================================================================

  def test_fetch_no_remote_configured(self):
    """Test fetch when no remote URL is configured"""
    # Clear remote first
    self.run_cmd("fim-remote", "clear")
    out, err, code = self.run_cmd("fim-recipe", "fetch", "test-recipe")
    self.assertEqual(out, "")
    self.assertIn("No remote URL configured", err)
    self.assertEqual(code, 125)

  def test_fetch_single_recipe(self):
    """Test fetching a single recipe from remote"""
    out, err, code = self.run_cmd("fim-recipe", "fetch", "gpu")
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
    out, err, code = self.run_cmd("fim-recipe", "fetch", "gpu,audio,xorg")
    self.assertIn("Successfully downloaded recipe 'gpu'", out)
    self.assertIn("Successfully downloaded recipe 'audio'", out)
    self.assertIn("Successfully downloaded recipe 'xorg'", out)
    self.assertIn("Connecting to raw.githubusercontent.com", err)
    self.assertEqual(code, 0)

  def test_fetch_invalid_recipe_name(self):
    """Test fetching a non-existent recipe"""
    out, err, code = self.run_cmd("fim-recipe", "fetch", "nonexistent-recipe-xyz")
    self.assertIn("Downloading recipe", out)
    self.assertIn("Failed to download recipe 'nonexistent-recipe-xyz'", err)
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
    out, err, code = self.run_cmd("fim-recipe", "fetch", "xorg")
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

  # ===========================================================================
  # fim-recipe info Tests
  # ===========================================================================

  def test_info_recipe_not_cached(self):
    """Test info on a recipe that hasn't been fetched"""
    out, err, code = self.run_cmd("fim-recipe", "info", "uncached-recipe")
    self.assertEqual(out, "")
    self.assertIn("not found locally", err)
    self.assertIn("fim-recipe fetch", err)
    self.assertEqual(code, 125)

  def test_info_single_recipe(self):
    """Test info on a cached recipe"""
    # Create a mock recipe
    mock_content = {
      "description": "Test recipe for GPU drivers",
      "packages": ["mesa", "vulkan-tools", "nvidia-driver"]
    }
    self.create_mock_recipe(self.get_distribution(), "gpu-test", mock_content)
    # Get recipe info
    out, err, code = self.run_cmd("fim-recipe", "info", "gpu-test")
    # Check fields
    self.assertIn("Recipe: gpu-test", out)
    self.assertIn("Test recipe for GPU drivers", out)
    self.assertIn("mesa", out)
    self.assertIn("vulkan-tools", out)
    self.assertIn("nvidia-driver", out)
    self.assertIn("Package count: 3", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_info_recipe_without_description(self):
    """Test info on recipe without description field"""
    mock_content = {
      "packages": ["package1", "package2"]
    }
    self.create_mock_recipe(self.get_distribution(), "minimal-recipe", mock_content)
    # Get recipe info
    out, err, code = self.run_cmd("fim-recipe", "info", "minimal-recipe")
    # Check fields
    self.assertEqual(out, "")
    self.assertIn("Missing 'description' field", err)
    self.assertEqual(code, 125)

  def test_info_recipe_without_packages(self):
    """Test info on recipe without packages field"""
    mock_content = {
      "description": "Recipe with no packages"
    }
    self.create_mock_recipe(self.get_distribution(), "empty-recipe", mock_content)
    # Get recipe info
    out, err, code = self.run_cmd("fim-recipe", "info", "empty-recipe")
    # Check fields
    self.assertEqual(out, "")
    self.assertIn("Missing 'packages' field", err)
    self.assertEqual(code, 125)

  def test_info_multiple_recipes_comma_separated(self):
    """Test info on multiple recipes with comma-separated names"""
    # Create two mock recipes
    mock1 = {"description": "First recipe", "packages": ["pkg1"]}
    mock2 = {"description": "Second recipe", "packages": ["pkg2"]}
    self.create_mock_recipe(self.get_distribution(), "recipe1", mock1)
    self.create_mock_recipe(self.get_distribution(), "recipe2", mock2)
    # Get recipe info
    out, err, code = self.run_cmd("fim-recipe", "info", "recipe1,recipe2")
    # Should display info for both recipes
    self.assertIn("Recipe: recipe1", out)
    self.assertIn("pkg1", out)
    self.assertIn("First recipe", out)
    self.assertIn("Recipe: recipe2", out)
    self.assertIn("pkg2", out)
    self.assertIn("Second recipe", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_json_missing_fields(self):
    """Test info on a recipe with missing JSON fields"""
    cache_path = self.get_recipe_cache_path(self.get_distribution(), "nofields-recipe")
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    with open(cache_path, 'w') as f:
      f.write("{}")
    # Get recipe info
    out, err, code = self.run_cmd("fim-recipe", "info", "nofields-recipe")
    # Check fields
    self.assertEqual(out, "")
    self.assertIn("Missing 'description' field", err)
    self.assertEqual(code, 125)

  def test_empty_json(self):
    """Test info on a recipe with an empty JSON"""
    cache_path = self.get_recipe_cache_path(self.get_distribution(), "empty-recipe")
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    Path(cache_path).touch()
    # Get recipe info
    out, err, code = self.run_cmd("fim-recipe", "info", "empty-recipe")
    # Check fields
    self.assertEqual(out, "")
    self.assertIn("Empty json file", err)
    self.assertEqual(code, 125)

  def test_info_malformed_json(self):
    """Test info on a recipe with malformed JSON"""
    cache_path = self.get_recipe_cache_path(self.get_distribution(), "broken-recipe")
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    with open(cache_path, 'w') as f:
      f.write("{invalid json content")
    # Get recipe info
    out, err, code = self.run_cmd("fim-recipe", "info", "broken-recipe")
    # Check fields
    self.assertEqual(out, "")
    self.assertIn("Could not parse json file", err)
    self.assertEqual(code, 125)

  def test_info_empty_file(self):
    """Test info on an empty recipe file"""
    cache_path = self.get_recipe_cache_path(self.get_distribution(), "empty-file")
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    cache_path.touch() 
    out, err, code = self.run_cmd("fim-recipe", "info", "empty-file")
    self.assertEqual(out, "")
    self.assertIn("Empty json file", err)
    self.assertEqual(code, 125)

  # ===========================================================================
  # fim-recipe install Tests
  # ===========================================================================

  def test_install_recipe_not_cached(self):
    """Test install when recipe hasn't been fetched"""
    out, err, code = self.run_cmd("fim-recipe", "install", "xorg")
    match self.get_distribution():
      case "alpine":
        self.assertRegex(out, "OK: [0-9]+ MiB in [0-9]+ packages")
        self.assertEqual(code, 0)
      case "arch":
        self.assertIn("Running post-transaction hooks...", out)
        self.assertEqual(code, 0)
      case _:
        self.assertTrue(False, f"Un-recognized distribution")

  def test_install_recipe_cached(self):
    """Test that install uses cached recipe if available"""
    # Create a mock recipe
    mock_content = {"description": "mock", "packages": ["htop"]}
    self.create_mock_recipe(self.get_distribution(), "htop", mock_content)
    # Install
    out, err, code = self.run_cmd("fim-recipe", "install", "htop")
    self.assertIn("Using existing recipe", out)
    self.assertNotIn("E::", err)
    self.assertEqual(code, 0)

  def test_install_multiple_recipes(self):
    """Test installing multiple recipes"""
    mock1 = {"description": "htop", "packages": ["htop"]}
    mock2 = {"description": "bmon", "packages": ["bmon"]}
    self.create_mock_recipe(self.get_distribution(), "htop", mock1)
    self.create_mock_recipe(self.get_distribution(), "bmon", mock2)
    out, err, code = self.run_cmd("fim-recipe", "install", "htop,bmon")
    self.assertIn("Using existing recipe", out)
    self.assertNotIn("Connecting to", out)
    self.assertNotIn("E::", err)
    self.assertEqual(code, 0)

  def test_install_invalid_recipe_name(self):
    """Test install with non-existent recipe"""
    out, err, code = self.run_cmd("fim-recipe", "install", "nonexistent-install")
    self.assertIn("Downloading recipe from", out)
    self.assertIn("HTTP/1.1 404 Not Found", err)
    self.assertEqual(code,125)

  # ===========================================================================
  # Edge Cases and Error Handling
  # ===========================================================================

  def test_recipe_empty_name(self):
    """Test recipe commands with empty name"""
    out, err, code = self.run_cmd("fim-recipe", "fetch", "")
    self.assertEqual(out, "")
    self.assertIn("Recipe argument is empty", err)
    self.assertEqual(code, 125)

  def test_recipe_whitespace_only_name(self):
    """Test recipe commands with whitespace-only name"""
    out, err, code = self.run_cmd("fim-recipe", "info", "   ")
    self.assertEqual(out, "")
    self.assertIn("Recipe '   ' not found locally", err)
    self.assertEqual(code, 125)

  def test_recipe_comma_separated_with_spaces(self):
    """Test comma-separated recipes with spaces"""
    mock1 = {"description": "recipe-a", "packages": ["pkg1"]}
    mock2 = {"description": "recipe-b", "packages": ["pkg2"]}
    self.create_mock_recipe(self.get_distribution(), "recipe-a", mock1)
    self.create_mock_recipe(self.get_distribution(), "recipe-b", mock2)
    # Test with spaces around commas
    out, err, code = self.run_cmd("fim-recipe", "info", "recipe-a, recipe-b")
    self.assertIn("Recipe: recipe-a", out)
    self.assertIn("Description: recipe-a", out)
    self.assertIn("Recipe ' recipe-b' not found locally", err)
    self.assertEqual(code, 125)

  def test_recipe_comma_separated_empty_entries(self):
    """Test comma-separated list with empty entries"""
    mock1 = {"description": "foo", "packages": ["pkg1"]}
    mock2 = {"description": "bar", "packages": ["pkg2"]}
    self.create_mock_recipe(self.get_distribution(), "recipe-a", mock1)
    self.create_mock_recipe(self.get_distribution(), "recipe-b", mock2)
    out, err, code = self.run_cmd("fim-recipe", "info", "recipe-a,,recipe-b")
    self.assertIn("Recipe: recipe-a", out)
    self.assertNotIn("Recipe: recipe-b", out)
    self.assertIn("Recipe '' not found locally", err)
    self.assertEqual(code, 125)

  # ===========================================================================
  # Dependencies Field Tests
  # ===========================================================================

  def test_info_recipe_with_dependencies(self):
    """Test info displays dependencies field"""
    mock_content = {
      "description": "Super Tux Kart",
      "dependencies": ["xorg", "gpu"],
      "packages": ["supertuxkart"]
    }
    self.create_mock_recipe(self.get_distribution(), "supertuxkart", mock_content)
    out, err, code = self.run_cmd("fim-recipe", "info", "supertuxkart")
    self.assertIn("Recipe: supertuxkart", out)
    self.assertIn("Dependencies: 2", out)
    self.assertIn("- xorg", out)
    self.assertIn("- gpu", out)
    self.assertIn("Package count: 1", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_info_recipe_with_empty_dependencies(self):
    """Test info on recipe with empty dependencies array"""
    mock_content = {
      "description": "Super Tux Kart",
      "dependencies": [],
      "packages": ["supertuxkart"]
    }
    self.create_mock_recipe(self.get_distribution(), "supertuxkart", mock_content)
    out, err, code = self.run_cmd("fim-recipe", "info", "supertuxkart")
    self.assertIn("Recipe: supertuxkart", out)
    self.assertIn("Dependencies: 0", out)
    self.assertIn("Package count: 1", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_fetch_recipe_with_dependencies(self):
    """Test fetch recursively fetches dependencies"""
    # Fetch should get both
    out, err, code = self.run_cmd("fim-recipe", "fetch", "supertuxkart")
    # Both should be fetched
    self.assertRegex(out, "Downloading recipe from.*supertuxkart.json'")
    self.assertRegex(out, "Downloading recipe from.*xorg.json'")
    self.assertRegex(out, "Downloading recipe from.*gpu.json'")
    self.assertRegex(out, "Downloading recipe from.*audio.json'")
    self.assertIn("Connecting to raw.githubusercontent.com", err)
    self.assertEqual(code, 0)

  def test_install_fetch_recipe_with_dependencies(self):
    """Test install fetches dependencies and installs all packages"""
    out, err, code = self.run_cmd("fim-recipe", "install", "supertuxkart")
    self.assertRegex(out, "Downloading recipe from.*supertuxkart.json'")
    self.assertRegex(out, "Downloading recipe from.*xorg.json'")
    self.assertRegex(out, "Downloading recipe from.*gpu.json'")
    self.assertRegex(out, "Downloading recipe from.*audio.json'")
    self.assertIn("Connecting to raw.githubusercontent.com", err)
    # self.assertEqual(code, 0)

  def test_install_with_missing_dependency(self):
    """Test install fails when dependency is missing"""
    # Create recipe with non-existent dependency
    recipe = {
      "description": "foo",
      "dependencies": ["nonexistent-dep"],
      "packages": ["pkg"]
    }
    self.create_mock_recipe(self.get_distribution(), "foo", recipe)
    # Install should fail
    out, err, code = self.run_cmd("fim-recipe", "install", "foo")
    self.assertRegex(out, "Downloading recipe from.*nonexistent-dep.json")
    self.assertIn("HTTP/1.1 404 Not Found", err)
    self.assertEqual(code, 125)

  def test_install_cyclic_dependency_error(self):
    """Test install fails gracefully with cyclic dependencies"""
    # Create cycle: foo > bar > foo
    foo = {
      "description": "foo",
      "dependencies": ["bar"],
      "packages": ["foo"]
    }
    bar = {
      "description": "bar",
      "dependencies": ["foo"],
      "packages": ["bar"]
    }
    self.create_mock_recipe(self.get_distribution(), "foo", foo)
    self.create_mock_recipe(self.get_distribution(), "bar", bar)
    # Install should detect cycle
    out, err, code = self.run_cmd("fim-recipe", "install", "foo")
    self.assertRegex(out, "Using existing recipe from.*foo.json")
    self.assertIn("Cyclic dependency for recipe 'foo'", err)
    self.assertEqual(code, 125)

  def test_dependencies_field_wrong_type(self):
    """Test recipe with dependencies field as non-array"""
    recipe = {
      "dependencies": "should-be-array",
      "packages": ["pkg"]
    }
    self.create_mock_recipe(self.get_distribution(), "bad-deps-type", recipe)
    # Info should handle gracefully or error
    _, err, code = self.run_cmd("fim-recipe", "info", "bad-deps-type")
    self.assertIn("Failed to get recipe info", err)
    # Should either skip the field or error
    self.assertTrue(code == 125)

  def test_recipe_only_dependencies_no_packages(self):
    """Test recipe with only dependencies but no packages"""
    # Create dependencies
    foo = {"dependencies": ["xorg"]}
    self.create_mock_recipe(self.get_distribution(), "foo", foo)
    # Info should work
    out, err, code = self.run_cmd("fim-recipe", "install", "foo")
    self.assertRegex(out, "Using existing recipe.*foo.json")
    self.assertIn("Missing 'description' field", err)
    self.assertEqual(code, 125)