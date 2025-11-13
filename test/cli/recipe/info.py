#!/bin/python3

import unittest
from pathlib import Path
from .common import RecipeTestBase
from cli.test_runner import run_cmd

class TestFimRecipeInfo(RecipeTestBase):
  """
  Test suite for fim-recipe info command:
  - Display cached recipe information
  - Validate recipe JSON structure
  - Handle missing or malformed recipes
  - Show dependencies information
  """

  # ===========================================================================
  # fim-recipe info Tests
  # ===========================================================================

  def test_info_recipe_not_cached(self):
    """Test info on a recipe that hasn't been fetched"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "uncached-recipe")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "gpu-test")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "minimal-recipe")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "empty-recipe")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "recipe1,recipe2")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "nofields-recipe")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "empty-recipe")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "broken-recipe")
    # Check fields
    self.assertEqual(out, "")
    self.assertIn("Could not parse json file", err)
    self.assertEqual(code, 125)

  def test_info_empty_file(self):
    """Test info on an empty recipe file"""
    cache_path = self.get_recipe_cache_path(self.get_distribution(), "empty-file")
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    cache_path.touch() 
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "empty-file")
    self.assertEqual(out, "")
    self.assertIn("Empty json file", err)
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "supertuxkart")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "supertuxkart")
    self.assertIn("Recipe: supertuxkart", out)
    self.assertIn("Dependencies: 0", out)
    self.assertIn("Package count: 1", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_dependencies_field_wrong_type(self):
    """Test recipe with dependencies field as non-array"""
    recipe = {
      "dependencies": "should-be-array",
      "packages": ["pkg"]
    }
    self.create_mock_recipe(self.get_distribution(), "bad-deps-type", recipe)
    # Info should handle gracefully or error
    _, err, code = run_cmd(self.file_image, "fim-recipe", "info", "bad-deps-type")
    self.assertIn("Failed to get recipe info", err)
    # Should either skip the field or error
    self.assertTrue(code == 125)
