#!/bin/python3

import os
import re
import shutil
from pathlib import Path
from .common import LayerTestBase
from cli.test_runner import run_cmd

class TestFimLayerList(LayerTestBase):
  """Test suite for fim-layer list command"""

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.file_layer_external = cls.dir_data / "external.layer"

  def tearDown(self):
    super().tearDown()
    if self.file_layer_external.exists():
      os.unlink(self.file_layer_external)

  def test_list_empty(self):
    """Test listing layers on a fresh image with only embedded layers"""
    # Get debug output to extract actual offsets
    os.environ["FIM_DEBUG"] = "1"
    debug_out, _, _ = run_cmd(self.file_image, "fim-exec", "echo", "test")
    os.environ["FIM_DEBUG"] = "0"
    # Parse debug output for layer mounting information
    # Format: "D::Mounting layer from '<filename>' with offset '<offset>'"
    debug_lines = debug_out.split('\n')
    expected_offsets = []
    for line in debug_lines:
      match = re.search(r"Mounting layer from '.*?' with offset '(\d+)'", line)
      if match:
        expected_offsets.append(int(match.group(1)))
    # Get layer list output
    out, err, code = run_cmd(self.file_image, "fim-layer", "list")
    self.assertEqual(code, 0)
    self.assertEqual(err, "")
    # Should have at least one embedded layer (the base system)
    lines = out.strip().split('\n')
    self.assertGreater(len(lines), 0)
    self.assertEqual(len(lines), len(expected_offsets), "Number of layers doesn't match debug output")
    # Check format: index:offset:size:path and validate against debug output
    for i, line in enumerate(lines):
      parts = line.split(':', 3)  # Split on first 3 colons (path may contain colons)
      self.assertEqual(len(parts), 4, f"Invalid format: {line}")
      # Index should match line number
      index = int(parts[0])
      self.assertEqual(index, i)
      # Offset should match what we got from debug output
      offset = int(parts[1])
      self.assertEqual(offset, expected_offsets[i],
                      f"Offset mismatch for layer {i}: expected {expected_offsets[i]}, got {offset}")
      # Size should be a positive integer
      size = int(parts[2])
      self.assertGreater(size, 0, f"Size should be positive for layer {i}")
      # Path should be non-empty
      self.assertGreater(len(parts[3]), 0)

  def test_list_after_commit(self):
    """Test listing layers after committing changes"""
    # Get initial layer count
    out_before, _, _ = run_cmd(self.file_image, "fim-layer", "list")
    initial_count = len(out_before.strip().split('\n'))
    # Create and commit a layer
    self.create_script("test layer 1")
    out, err, code = run_cmd(self.file_image, "fim-layer", "commit", "binary")
    self.assertEqual(code, 0)
    self.assertIn("Filesystem appended to binary", out)
    # List layers again
    out_after, err, code = run_cmd(self.file_image, "fim-layer", "list")
    self.assertEqual(code, 0)
    self.assertEqual(err, "")
    # Should have one more layer
    lines = out_after.strip().split('\n')
    self.assertEqual(len(lines), initial_count + 1)
    # The new layer should be at the end with increasing index
    last_line = lines[-1]
    parts = last_line.split(':', 3)
    self.assertEqual(int(parts[0]), initial_count)

  def test_list_multiple_commits(self):
    """Test listing layers after multiple commits"""
    # Get initial layer count
    out_before, _, _ = run_cmd(self.file_image, "fim-layer", "list")
    initial_count = len(out_before.strip().split('\n'))
    # Commit multiple layers
    num_commits = 3
    for i in range(num_commits):
      self.create_script(f"test layer {i}")
      out, err, code = run_cmd(self.file_image, "fim-layer", "commit", "binary")
      self.assertEqual(code, 0)
    # List layers
    out, err, code = run_cmd(self.file_image, "fim-layer", "list")
    self.assertEqual(code, 0)
    self.assertEqual(err, "")
    # Should have all committed layers
    lines = out.strip().split('\n')
    self.assertEqual(len(lines), initial_count + num_commits)
    # Verify indices are sequential
    for i, line in enumerate(lines):
      parts = line.split(':', 3)
      self.assertEqual(int(parts[0]), i)

  def test_list_with_external_layer(self):
    """Test listing with external layer files via FIM_LAYERS"""
    # Get initial layer count
    out_before, _, _ = run_cmd(self.file_image, "fim-layer", "list")
    initial_count = len(out_before.strip().split('\n'))
    # Create an external layer file
    self.create_script("external layer content")
    out, err, code = run_cmd(
      self.file_image,
      "fim-layer",
      "create",
      str(self.dir_image / "root"),
      str(self.file_layer_external)
    )
    self.assertEqual(code, 0)
    self.assertIn("Filesystem created without errors", out)
    # Set FIM_LAYERS to load the external layer
    os.environ["FIM_LAYERS"] = str(self.file_layer_external)
    # List layers with the external layer loaded
    out, err, code = run_cmd(self.file_image, "fim-layer", "list")
    self.assertEqual(code, 0)
    self.assertEqual(err, "")
    # Clean up environment variable
    del os.environ["FIM_LAYERS"]
    # Should have the external layer
    lines = out.strip().split('\n')
    self.assertEqual(len(lines), initial_count + 1)
    # The last layer should be the external one with offset 0
    last_line = lines[-1]
    parts = last_line.split(':', 3)
    self.assertEqual(int(parts[0]), initial_count)
    # External layers should have offset 0
    self.assertEqual(int(parts[1]), 0)
    # Size should match the actual file size
    expected_size = self.file_layer_external.stat().st_size
    self.assertEqual(int(parts[2]), expected_size)
    # Path should match the external layer file
    self.assertIn(self.file_layer_external.name, parts[3])

  def test_list_format_validation(self):
    """Test that list output format is correct"""
    out, err, code = run_cmd(self.file_image, "fim-layer", "list")
    self.assertEqual(code, 0)
    self.assertEqual(err, "")
    # Each line should have exactly 4 parts separated by colons
    lines = out.strip().split('\n')
    for line in lines:
      if not line:  # Skip empty lines
        continue
      parts = line.split(':', 3)  # Split on first 3 colons only (path may contain colons)
      self.assertEqual(len(parts), 4, f"Invalid format: {line}")
      # Index should be a non-negative integer
      try:
        index = int(parts[0])
        self.assertGreaterEqual(index, 0)
      except ValueError:
        self.fail(f"Index is not a valid integer: {parts[0]}")
      # Offset should be a non-negative integer
      try:
        offset = int(parts[1])
        self.assertGreaterEqual(offset, 0)
      except ValueError:
        self.fail(f"Offset is not a valid integer: {parts[1]}")
      # Size should be a positive integer
      try:
        size = int(parts[2])
        self.assertGreater(size, 0)
      except ValueError:
        self.fail(f"Size is not a valid integer: {parts[2]}")
      # Path should be non-empty
      self.assertGreater(len(parts[3]), 0, "Path is empty")

  def test_list_with_managed_layers(self):
    """Test listing with layers in the managed layers directory"""
    # Get initial layer count
    out_before, _, _ = run_cmd(self.file_image, "fim-layer", "list")
    initial_count = len(out_before.strip().split('\n'))
    # Create a managed layer
    self.create_script("managed layer content")
    out, _, code = run_cmd(self.file_image, "fim-layer", "commit", "layer")
    self.assertEqual(code, 0)
    self.assertIn("Layer saved to", out)
    # List layers
    out, _, code = run_cmd(self.file_image, "fim-layer", "list")
    self.assertEqual(code, 0)
    # The managed layer should be included in the list
    lines = out.strip().split('\n')
    # Should have at least one more layer than before
    self.assertEqual(len(lines), initial_count+1)
    # Verify at least one layer has offset 0 (external layer)
    has_external = any(line.split(':', 3)[1] == '0' for line in lines if line)
    self.assertTrue(has_external, "No external layer found with offset 0")

  def test_list_mixed_layers(self):
    """Test listing with a mix of embedded, external (FIM_LAYERS), and managed layers"""
    # Get initial embedded layer count
    out_initial, _, _ = run_cmd(self.file_image, "fim-layer", "list")
    initial_embedded_count = len(out_initial.strip().split('\n'))
    # Step 1: Commit a layer to binary (new embedded layer)
    self.create_script("embedded layer 1")
    out, err, code = run_cmd(self.file_image, "fim-layer", "commit", "binary")
    self.assertEqual(code, 0)
    self.assertIn("Filesystem appended to binary", out)
    shutil.rmtree(self.dir_image / "root", ignore_errors=True)
    # Step 2: Create a managed layer (saved to layers directory)
    self.create_script("managed layer 1")
    out, err, code = run_cmd(self.file_image, "fim-layer", "commit", "layer")
    self.assertEqual(code, 0)
    self.assertIn("Layer saved to", out)
    shutil.rmtree(self.dir_image / "root", ignore_errors=True)
    # Step 3: Create an external layer file (via FIM_LAYERS)
    external_layer = self.dir_data / "external_test.layer"
    self.create_script("external layer 1")
    out, err, code = run_cmd(
      self.file_image,
      "fim-layer",
      "create",
      str(self.dir_image / "root"),
      str(external_layer)
    )
    self.assertEqual(code, 0)
    self.assertIn("Filesystem created without errors", out)
    # Set FIM_LAYERS to include the external layer
    os.environ["FIM_LAYERS"] = str(external_layer)
    # List all layers
    out, err, code = run_cmd(self.file_image, "fim-layer", "list")
    self.assertEqual(code, 0)
    self.assertEqual(err, "")
    # Clean up
    del os.environ["FIM_LAYERS"]
    if external_layer.exists():
      os.unlink(external_layer)
    # Verify layer composition
    lines = out.strip().split('\n')
    # Should have: initial embedded + 1 new embedded + 1 managed + 1 external = initial + 3
    self.assertEqual(len(lines), initial_embedded_count + 3,
                    f"Expected {initial_embedded_count + 3} total layers, got {len(lines)}")
    # Verify indices are sequential
    for i, line in enumerate(lines):
      parts = line.split(':', 3)
      self.assertEqual(int(parts[0]), i, f"Index mismatch at line {i}")
    # Check offsets:
    # - Initial embedded layers should have offsets > 0
    # - New embedded layer should have offset > 0
    # - Managed layer (external) should have offset 0
    # - External layer should have offset 0
    offset_types = {"embedded": 0, "external": 0}
    for line in lines:
      parts = line.split(':', 3)
      offset = int(parts[1])
      if offset > 0:
        offset_types["embedded"] += 1
      else:
        offset_types["external"] += 1
    # Should have at least the initial embedded layers + 1 new embedded
    self.assertGreaterEqual(offset_types["embedded"], initial_embedded_count + 1)
    # Should have exactly 2 external layers (managed + FIM_LAYERS)
    self.assertEqual(offset_types["external"], 2,
                    f"Expected 2 external layers, got {offset_types['external']}")

  def test_list_no_extra_arguments(self):
    """Test that list command doesn't accept extra arguments"""
    out, err, code = run_cmd(self.file_image, "fim-layer", "list", "extra", "arguments")
    self.assertEqual(code, 125)
    self.assertIn("Trailing arguments for fim-layer list", err)
