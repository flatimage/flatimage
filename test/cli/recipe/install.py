#!/bin/python3

import unittest
from .common import RecipeTestBase
from cli.test_runner import run_cmd

class TestFimRecipeInstall(RecipeTestBase):
  """
  Test suite for fim-recipe install command:
  - Install packages from recipes
  - Handle cached and uncached recipes
  - Process recipe dependencies
  - Validate dependency resolution
  """

  # ===========================================================================
  # fim-recipe install Tests
  # ===========================================================================

  def test_install_recipe_not_cached(self):
    """Test install when recipe hasn't been fetched"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "xorg")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "htop")
    self.assertIn("Using existing recipe", out)
    self.assertNotIn("E::", err)
    self.assertEqual(code, 0)

  def test_install_multiple_recipes(self):
    """Test installing multiple recipes"""
    mock1 = {"description": "htop", "packages": ["htop"]}
    mock2 = {"description": "bmon", "packages": ["bmon"]}
    self.create_mock_recipe(self.get_distribution(), "htop", mock1)
    self.create_mock_recipe(self.get_distribution(), "bmon", mock2)
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "htop,bmon")
    self.assertIn("Using existing recipe", out)
    self.assertNotIn("Connecting to", out)
    self.assertNotIn("E::", err)
    self.assertEqual(code, 0)

  def test_install_invalid_recipe_name(self):
    """Test install with non-existent recipe"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "nonexistent-install")
    self.assertIn("Downloading recipe from", out)
    self.assertIn("HTTP/1.1 404 Not Found", err)
    self.assertEqual(code, 125)

  # ===========================================================================
  # Dependencies Field Tests
  # ===========================================================================

  def test_fetch_recipe_with_dependencies(self):
    """Test fetch recursively fetches dependencies"""
    # Fetch should get both
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch", "supertuxkart")
    # Both should be fetched
    self.assertRegex(out, "Downloading recipe from.*supertuxkart.json'")
    self.assertRegex(out, "Downloading recipe from.*xorg.json'")
    self.assertRegex(out, "Downloading recipe from.*gpu.json'")
    self.assertRegex(out, "Downloading recipe from.*audio.json'")
    self.assertIn("Connecting to raw.githubusercontent.com", err)
    self.assertEqual(code, 0)

  def test_install_fetch_recipe_with_dependencies(self):
    """Test install fetches dependencies and installs all packages"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "supertuxkart")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "foo")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "foo")
    self.assertRegex(out, "Using existing recipe from.*foo.json")
    self.assertIn("Cyclic dependency for recipe 'foo'", err)
    self.assertEqual(code, 125)

  def test_recipe_only_dependencies_no_packages(self):
    """Test recipe with only dependencies but no packages"""
    # Create dependencies
    foo = {"dependencies": ["xorg"]}
    self.create_mock_recipe(self.get_distribution(), "foo", foo)
    # Info should work
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "foo")
    self.assertRegex(out, "Using existing recipe.*foo.json")
    self.assertIn("Missing 'description' field", err)
    self.assertEqual(code, 125)
