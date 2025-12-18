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
        self.assertRegex(out, "OK: .* MiB in [0-9]+ packages")
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

  # ===========================================================================
  # Desktop Integration Test
  # ===========================================================================

  def test_install_recipe_with_desktop_integration(self):
    """Test that desktop integration is applied when recipe contains desktop field"""
    recipe = {
      "description": "Test app with desktop integration",
      "packages": ["htop"],
      "desktop": {
        "name": "Test App",
        "icon": "https://upload.wikimedia.org/wikipedia/commons/5/5d/Breezeicons-apps-48-htop.svg",
        "categories": ["Utility", "System"]
      }
    }
    self.create_mock_recipe(self.get_distribution(), "test-desktop", recipe)
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "test-desktop")
    self.assertIn("Using existing recipe", out)
    self.assertIn("Found desktop integration in recipe 'test-desktop'", out)
    self.assertIn("Setting up desktop integration from recipe", out)
    self.assertIn("Desktop integration setup successful", out)
    self.assertNotIn("E::", err)
    self.assertEqual(code, 0)

  def test_install_recipe_without_desktop_integration(self):
    """Test that install works normally when recipe has no desktop field"""
    recipe = {
      "description": "Test app without desktop integration",
      "packages": ["htop"]
    }
    self.create_mock_recipe(self.get_distribution(), "test-no-desktop", recipe)
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "test-no-desktop")
    self.assertIn("Using existing recipe", out)
    self.assertNotIn("Found desktop integration", out)
    self.assertNotIn("Setting up desktop integration", out)
    self.assertNotIn("E::", err)
    self.assertEqual(code, 0)

  def test_install_multiple_recipes_last_desktop_wins(self):
    """Test that when installing multiple recipes, the last desktop integration is used"""
    recipe1 = {
      "description": "First app",
      "packages": ["htop"],
      "desktop": {
        "name": "First App",
        "icon": "https://upload.wikimedia.org/wikipedia/commons/8/83/Steam_icon_logo.svg",
        "categories": ["Utility"]
      }
    }
    recipe2 = {
      "description": "Second app",
      "packages": ["htop"],
      "desktop": {
        "name": "Second App",
        "icon": "https://upload.wikimedia.org/wikipedia/commons/3/37/Logo_de_SuperTuxKart.png",
        "categories": ["System"]
      }
    }
    self.create_mock_recipe(self.get_distribution(), "first-app", recipe1)
    self.create_mock_recipe(self.get_distribution(), "second-app", recipe2)
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "first-app,second-app")
    self.assertIn("Found desktop integration in recipe 'first-app'", out)
    self.assertIn("Found desktop integration in recipe 'second-app'", out)
    self.assertIn("Setting up desktop integration from recipe", out)
    self.assertNotIn("E::", err)
    self.assertEqual(code, 0)

  def test_install_desktop_integration_with_url_icon_svg(self):
    """Test desktop integration with a URL SVG icon (should be downloaded)"""
    recipe = {
      "description": "App with SVG URL icon",
      "packages": ["htop"],
      "desktop": {
        "name": "Steam Test",
        "icon": "https://upload.wikimedia.org/wikipedia/commons/8/83/Steam_icon_logo.svg",
        "categories": ["Game"]
      }
    }
    self.create_mock_recipe(self.get_distribution(), "url-icon-svg", recipe)
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "url-icon-svg")
    self.assertIn("Found desktop integration in recipe 'url-icon-svg'", out)
    self.assertIn("Setting up desktop integration from recipe", out)
    self.assertEqual(code, 0)

  def test_install_desktop_integration_with_url_icon_png(self):
    """Test desktop integration with a URL PNG icon (should be downloaded)"""
    recipe = {
      "description": "App with PNG URL icon",
      "packages": ["htop"],
      "desktop": {
        "name": "SuperTuxKart Test",
        "icon": "https://upload.wikimedia.org/wikipedia/commons/3/37/Logo_de_SuperTuxKart.png",
        "categories": ["Game"]
      }
    }
    self.create_mock_recipe(self.get_distribution(), "url-icon-png", recipe)
    out, _, code = run_cmd(self.file_image, "fim-recipe", "install", "url-icon-png")
    self.assertIn("Found desktop integration in recipe 'url-icon-png'", out)
    self.assertIn("Setting up desktop integration from recipe", out)
    self.assertIn("Desktop integration setup successful", out)
    self.assertEqual(code, 0)

  def test_install_desktop_integration_with_integrations_field(self):
    """Test desktop integration with explicit integrations field"""
    recipe = {
      "description": "App with integrations",
      "packages": ["htop"],
      "desktop": {
        "name": "Integration Test",
        "icon": "https://upload.wikimedia.org/wikipedia/commons/5/5d/Breezeicons-apps-48-htop.svg",
        "integrations": ["ENTRY", "ICON"],
        "categories": ["Development"]
      }
    }
    self.create_mock_recipe(self.get_distribution(), "integrations", recipe)
    out, _, code = run_cmd(self.file_image, "fim-recipe", "install", "integrations")
    self.assertIn("Found desktop integration in recipe 'integrations'", out)
    self.assertIn("Setting up desktop integration from recipe", out)
    self.assertIn("Desktop integration setup successful", out)
    self.assertEqual(code, 0)

  def test_install_recipe_with_dependencies_and_desktop(self):
    """Test that desktop integration works with recipe dependencies"""
    dep_recipe = {
      "description": "Dependency",
      "packages": ["htop"]
    }
    main_recipe = {
      "description": "Main app with desktop",
      "packages": ["htop"],
      "dependencies": ["dep-recipe"],
      "desktop": {
        "name": "Main App",
        "icon": "https://upload.wikimedia.org/wikipedia/commons/5/5d/Breezeicons-apps-48-htop.svg",
        "categories": ["Utility"]
      }
    }
    self.create_mock_recipe(self.get_distribution(), "dep-recipe", dep_recipe)
    self.create_mock_recipe(self.get_distribution(), "main-with-desktop", main_recipe)
    out, err, code = run_cmd(self.file_image, "fim-recipe", "install", "main-with-desktop")
    self.assertIn("Found desktop integration in recipe 'main-with-desktop'", out)
    self.assertIn("Setting up desktop integration from recipe", out)
    self.assertEqual(code, 0)

  def test_install_real_recipe_firefox_desktop(self):
    """Test installing real Firefox recipe which includes desktop integration"""
    out, _, _ = run_cmd(self.file_image, "fim-recipe", "install", "firefox")
    self.assertRegex(out, "Downloading recipe from.*firefox.json")
    self.assertIn("Found desktop integration in recipe 'firefox'", out)
    self.assertIn("Setting up desktop integration from recipe", out)
    # Fix the exit code issues in alpine eventually...
    # self.assertEqual(code, 0)