#!/bin/python3

import os
import shutil
import unittest
from pathlib import Path
from cli.test_runner import run_cmd

class TestFimLayers(unittest.TestCase):
  """
  Tests for FIM_LAYERS feature - unified layer loading from files and directories.

  The FIM_LAYERS environment variable accepts a colon-separated list of paths
  that can be either individual layer files or directories containing layer files.
  This unified interface replaces the legacy FIM_DIRS_LAYER and FIM_FILES_LAYER.
  """

  @classmethod
  def setUpClass(cls):
    # Path to the current script
    cls.dir_script = Path(__file__).resolve().parent.parent
    # FlatImage and its data directories
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.file_image_src = os.environ["FILE_IMAGE_SRC"]
    cls.dir_data = Path(os.environ["DIR_DATA"])
    cls.dir_image = Path(os.environ["DIR_IMAGE"])

    # Create directories for layers
    cls.dir_layers = cls.dir_data / "layers"
    cls.dir_layers_base = cls.dir_layers / "base"
    cls.dir_layers_apps = cls.dir_layers / "apps"

  def setUp(self):
    # Ensure data directories exist
    self.dir_data.mkdir(parents=True, exist_ok=True)
    self.dir_layers.mkdir(parents=True, exist_ok=True)
    self.dir_layers_base.mkdir(parents=True, exist_ok=True)
    self.dir_layers_apps.mkdir(parents=True, exist_ok=True)

    # Copy fresh image and chmod +x
    shutil.copy(self.file_image_src, self.file_image)
    os.chmod(self.file_image, 0o755)

  def tearDown(self):
    # Remove image file
    if Path(self.file_image).exists():
      os.unlink(self.file_image)
    # Remove data directories
    shutil.rmtree(self.dir_data, ignore_errors=True)
    # Remove default data directory
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def _create_test_layer(self, layer_path: Path, marker_file: str, marker_content: str):
    """
    Helper function to create a test layer file.

    Creates a layer by:
    1. Writing a marker file to the overlay
    2. Committing the overlay to a layer file

    Args:
      layer_path: Path where the layer file should be created
      marker_file: Path inside the container for the marker file
      marker_content: Content to write to the marker file
    """
    env = os.environ.copy()
    env["FIM_DIR_DATA"] = str(self.dir_data / "temp_layer_creation")
    # Create marker file in overlay
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "sh", "-c",
      f"mkdir -p $(dirname {marker_file}) && echo '{marker_content}' > {marker_file}",
      env=env
    )
    self.assertEqual(code, 0, f"Failed to create marker file: {err}")
    # Commit to layer file
    out, err, code = run_cmd(
      self.file_image,
      "fim-layer", "commit", "file", str(layer_path),
      env=env
    )
    self.assertEqual(code, 0, f"Failed to commit layer: {err}")
    # Cleanup temp data directory
    shutil.rmtree(self.dir_data / "temp_layer_creation", ignore_errors=True)

  def test_load_single_layer_file(self):
    """
    Test loading a single layer file via FIM_LAYERS.

    This test:
    1. Creates a layer file with a marker
    2. Loads it via FIM_LAYERS
    3. Verifies the marker is accessible
    """
    layer_file = self.dir_layers / "test_layer.layer"
    marker_file = "/test_single/marker.txt"
    marker_content = "single_layer_marker"
    # Create test layer
    self._create_test_layer(layer_file, marker_file, marker_content)
    # Load layer and verify marker
    env = os.environ.copy()
    env["FIM_LAYERS"] = str(layer_file)
    # Cat the layer file
    out, err, code = run_cmd(self.file_image, "fim-exec", "cat", marker_file, env=env)
    self.assertEqual(code, 0, f"Failed to read marker from layer: {err}")
    self.assertIn(marker_content, out)

  def test_load_multiple_layer_files(self):
    """
    Test loading multiple layer files via FIM_LAYERS with colon separation.

    This test:
    1. Creates two layer files with different markers
    2. Loads both via FIM_LAYERS=layer1:layer2
    3. Verifies both markers are accessible
    """
    layer_file_1 = self.dir_layers / "layer_1.layer"
    layer_file_2 = self.dir_layers / "layer_2.layer"
    marker_file_1 = "/test_multi/marker_1.txt"
    marker_file_2 = "/test_multi/marker_2.txt"
    marker_content_1 = "first_layer_marker"
    marker_content_2 = "second_layer_marker"
    # Create test layers
    self._create_test_layer(layer_file_1, marker_file_1, marker_content_1)
    self._create_test_layer(layer_file_2, marker_file_2, marker_content_2)
    # Load both layers
    env = os.environ.copy()
    env["FIM_LAYERS"] = f"{layer_file_1}:{layer_file_2}"
    # Verify first marker
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "cat", marker_file_1,
      env=env
    )
    self.assertEqual(code, 0, f"Failed to read marker 1: {err}")
    self.assertIn(marker_content_1, out)
    # Verify second marker
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "cat", marker_file_2,
      env=env
    )
    self.assertEqual(code, 0, f"Failed to read marker 2: {err}")
    self.assertIn(marker_content_2, out)

  def test_load_directory_of_layers(self):
    """
    Test loading all layers from a directory via FIM_LAYERS.

    This test:
    1. Creates multiple layer files in a directory
    2. Loads the directory via FIM_LAYERS
    3. Verifies all layers are mounted (alphabetical order)
    """
    # Create three layers with alphabetically ordered names
    layer_a = self.dir_layers_base / "01-layer-a.layer"
    layer_b = self.dir_layers_base / "02-layer-b.layer"
    layer_c = self.dir_layers_base / "03-layer-c.layer"
    self._create_test_layer(layer_a, "/test_dir/marker_a.txt", "marker_a_content")
    self._create_test_layer(layer_b, "/test_dir/marker_b.txt", "marker_b_content")
    self._create_test_layer(layer_c, "/test_dir/marker_c.txt", "marker_c_content")
    # Load directory
    env = os.environ.copy()
    env["FIM_LAYERS"] = str(self.dir_layers_base)
    # Verify all three markers
    for marker, content in [
      ("/test_dir/marker_a.txt", "marker_a_content"),
      ("/test_dir/marker_b.txt", "marker_b_content"),
      ("/test_dir/marker_c.txt", "marker_c_content"),
    ]:
      out, err, code = run_cmd(
        self.file_image,
        "fim-exec", "cat", marker,
        env=env
      )
      self.assertEqual(code, 0, f"Failed to read {marker}: {err}")
      self.assertIn(content, out)

  def test_load_mixed_directories_and_files(self):
    """
    Test loading a mix of directories and files via FIM_LAYERS.

    This test:
    1. Creates layers in a directory
    2. Creates a standalone layer file
    3. Loads both via FIM_LAYERS=directory:file
    4. Verifies all layers are accessible
    """
    # Create directory with two layers
    layer_dir_1 = self.dir_layers_base / "base-layer-1.layer"
    layer_dir_2 = self.dir_layers_base / "base-layer-2.layer"
    self._create_test_layer(layer_dir_1, "/test_mixed/base_1.txt", "base_1_content")
    self._create_test_layer(layer_dir_2, "/test_mixed/base_2.txt", "base_2_content")
    # Create standalone layer file
    layer_standalone = self.dir_layers / "standalone.layer"
    self._create_test_layer(layer_standalone, "/test_mixed/standalone.txt", "standalone_content")
    # Load directory + standalone file
    env = os.environ.copy()
    env["FIM_LAYERS"] = f"{self.dir_layers_base}:{layer_standalone}"
    # Verify all markers
    for marker, content in [
      ("/test_mixed/base_1.txt", "base_1_content"),
      ("/test_mixed/base_2.txt", "base_2_content"),
      ("/test_mixed/standalone.txt", "standalone_content"),
    ]:
      out, err, code = run_cmd(
        self.file_image,
        "fim-exec", "cat", marker,
        env=env
      )
      self.assertEqual(code, 0, f"Failed to read {marker}: {err}")
      self.assertIn(content, out)

  def test_layer_precedence_files_override_directories(self):
    """
    Test that later layers override earlier layers.

    This test:
    1. Creates a layer in a directory with a file
    2. Creates a standalone layer that overwrites the same file
    3. Loads directory:file
    4. Verifies the file content matches the later layer
    """
    # Create base layer with initial content
    layer_base = self.dir_layers_base / "base.layer"
    self._create_test_layer(layer_base, "/test_precedence/config.txt", "base_config")
    # Create override layer with different content
    layer_override = self.dir_layers / "override.layer"
    self._create_test_layer(layer_override, "/test_precedence/config.txt", "override_config")
    # Load base directory first, then override file
    env = os.environ.copy()
    env["FIM_LAYERS"] = f"{self.dir_layers_base}:{layer_override}"
    # Verify override content wins
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "cat", "/test_precedence/config.txt",
      env=env
    )
    self.assertEqual(code, 0, f"Failed to read config: {err}")
    self.assertIn("override_config", out)
    self.assertNotIn("base_config", out)

  def test_multiple_directories(self):
    """
    Test loading layers from multiple directories via FIM_LAYERS.

    This test:
    1. Creates layers in two separate directories
    2. Loads both directories via FIM_LAYERS=dir1:dir2
    3. Verifies layers from both directories are accessible
    """
    # Create layers in first directory
    layer_base_1 = self.dir_layers_base / "layer-1.layer"
    self._create_test_layer(layer_base_1, "/test_multidirs/base.txt", "base_content")
    # Create layers in second directory
    layer_app_1 = self.dir_layers_apps / "app-1.layer"
    self._create_test_layer(layer_app_1, "/test_multidirs/app.txt", "app_content")
    # Load both directories
    env = os.environ.copy()
    env["FIM_LAYERS"] = f"{self.dir_layers_base}:{self.dir_layers_apps}"
    # Verify both markers
    for marker, content in [
      ("/test_multidirs/base.txt", "base_content"),
      ("/test_multidirs/app.txt", "app_content"),
    ]:
      out, err, code = run_cmd(
        self.file_image,
        "fim-exec", "cat", marker,
        env=env
      )
      self.assertEqual(code, 0, f"Failed to read {marker}: {err}")
      self.assertIn(content, out)

  def test_empty_fim_layers(self):
    """
    Test that FlatImage works correctly when FIM_LAYERS is empty or unset.

    This test:
    1. Runs FlatImage with FIM_LAYERS=""
    2. Runs FlatImage with FIM_LAYERS unset
    3. Verifies both work without errors
    """
    # Test with empty string
    env = os.environ.copy()
    env["FIM_LAYERS"] = ""

    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "echo", "test_empty_layers",
      env=env
    )
    self.assertEqual(code, 0, f"Failed with empty FIM_LAYERS: {err}")
    self.assertIn("test_empty_layers", out)
    # Test with unset variable
    env = os.environ.copy()
    if "FIM_LAYERS" in env:
      del env["FIM_LAYERS"]
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "echo", "test_unset_layers",
      env=env
    )
    self.assertEqual(code, 0, f"Failed with unset FIM_LAYERS: {err}")
    self.assertIn("test_unset_layers", out)

  def test_nonexistent_layer_file_graceful_handling(self):
    """
    Test that nonexistent layer files are handled gracefully (skipped with warning).

    This test:
    1. Creates a valid layer
    2. References a nonexistent layer in FIM_LAYERS
    3. Verifies the valid layer is still loaded
    """
    # Create valid layer
    layer_valid = self.dir_layers / "valid.layer"
    self._create_test_layer(layer_valid, "/test_nonexist/valid.txt", "valid_content")
    # Reference both valid and nonexistent layers
    layer_nonexistent = self.dir_layers / "does_not_exist.layer"
    env = os.environ.copy()
    env["FIM_LAYERS"] = f"{layer_valid}:{layer_nonexistent}"
    # Should still work with valid layer
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "cat", "/test_nonexist/valid.txt",
      env=env
    )
    self.assertEqual(code, 0, f"Failed to read from valid layer: {err}")
    self.assertIn("valid_content", out)

  def test_layer_ordering_alphabetical_in_directories(self):
    """
    Test that layers within a directory are loaded in alphabetical order.

    This test:
    1. Creates layers with names that ensure a specific load order
    2. Uses numeric prefixes to control ordering
    3. Verifies the correct precedence (later overrides earlier)
    """
    # Create three layers with the same file but different content
    # Alphabetical order: 01 -> 02 -> 03
    layer_01 = self.dir_layers_base / "01-first.layer"
    layer_02 = self.dir_layers_base / "02-second.layer"
    layer_03 = self.dir_layers_base / "03-third.layer"
    self._create_test_layer(layer_01, "/test_order/config.txt", "first")
    self._create_test_layer(layer_02, "/test_order/config.txt", "second")
    self._create_test_layer(layer_03, "/test_order/config.txt", "third")
    # Load directory - should apply in alphabetical order
    env = os.environ.copy()
    env["FIM_LAYERS"] = str(self.dir_layers_base)
    # Last layer (03) should win
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "cat", "/test_order/config.txt",
      env=env
    )
    self.assertEqual(code, 0, f"Failed to read config: {err}")
    self.assertIn("third", out)
    self.assertNotIn("first", out)
    self.assertNotIn("second", out)

  def test_complex_mixed_scenario(self):
    """
    Test a complex real-world scenario with multiple directories and files mixed.

    This test simulates a realistic use case:
    1. Base system layers in one directory
    2. Application layers in another directory
    3. Custom override layer as a standalone file
    """
    # Create base layers
    layer_base_system = self.dir_layers_base / "01-system.layer"
    layer_base_libs = self.dir_layers_base / "02-libs.layer"
    self._create_test_layer(layer_base_system, "/test_complex/system.txt", "system_layer")
    self._create_test_layer(layer_base_libs, "/test_complex/libs.txt", "libs_layer")
    # Create application layers
    layer_app_firefox = self.dir_layers_apps / "firefox.layer"
    self._create_test_layer(layer_app_firefox, "/test_complex/firefox.txt", "firefox_layer")
    # Create custom override
    layer_custom = self.dir_layers / "custom-override.layer"
    self._create_test_layer(layer_custom, "/test_complex/custom.txt", "custom_layer")
    # Load all: base_dir:apps_dir:custom_file
    env = os.environ.copy()
    env["FIM_LAYERS"] = f"{self.dir_layers_base}:{self.dir_layers_apps}:{layer_custom}"
    # Verify all markers are accessible
    for marker, content in [
      ("/test_complex/system.txt", "system_layer"),
      ("/test_complex/libs.txt", "libs_layer"),
      ("/test_complex/firefox.txt", "firefox_layer"),
      ("/test_complex/custom.txt", "custom_layer"),
    ]:
      out, err, code = run_cmd(
        self.file_image,
        "fim-exec", "cat", marker,
        env=env
      )
      self.assertEqual(code, 0, f"Failed to read {marker}: {err}")
      self.assertIn(content, out)
