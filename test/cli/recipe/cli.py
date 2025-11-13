#!/bin/python3

from .common import RecipeTestBase
from cli.test_runner import run_cmd

class TestFimRecipeCLI(RecipeTestBase):
  """
  Test suite for fim-recipe CLI validation:
  - Command validation
  - Argument validation
  - Error handling for missing/invalid arguments
  """

  # ===========================================================================
  # CLI Validation Tests
  # ===========================================================================

  def test_recipe_no_subcommand(self):
    """Test fim-recipe with no subcommand"""
    out, err, code = run_cmd(self.file_image, "fim-recipe")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-recipe'", err)
    self.assertEqual(code, 125)

  def test_recipe_invalid_subcommand(self):
    """Test fim-recipe with invalid subcommand"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "invalid-cmd")
    self.assertEqual(out, "")
    self.assertIn("Invalid recipe operation", err)
    self.assertEqual(code, 125)

  def test_recipe_missing_recipe_name(self):
    """Test fim-recipe fetch with no recipe name"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch")
    self.assertEqual(out, "")
    self.assertIn("Missing recipe", err)
    self.assertEqual(code, 125)

  def test_recipe_trailing_arguments(self):
    """Test fim-recipe with trailing unexpected arguments"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch", "test-recipe", "extra-arg")
    self.assertEqual(out, "")
    self.assertIn("Trailing arguments", err)
    self.assertEqual(code, 125)

  # ===========================================================================
  # Edge Cases and Error Handling
  # ===========================================================================

  def test_recipe_empty_name(self):
    """Test recipe commands with empty name"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "fetch", "")
    self.assertEqual(out, "")
    self.assertIn("Recipe argument is empty", err)
    self.assertEqual(code, 125)

  def test_recipe_whitespace_only_name(self):
    """Test recipe commands with whitespace-only name"""
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "   ")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "recipe-a, recipe-b")
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
    out, err, code = run_cmd(self.file_image, "fim-recipe", "info", "recipe-a,,recipe-b")
    self.assertIn("Recipe: recipe-a", out)
    self.assertNotIn("Recipe: recipe-b", out)
    self.assertIn("Recipe '' not found locally", err)
    self.assertEqual(code, 125)
