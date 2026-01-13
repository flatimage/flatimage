#!/bin/python3

import os
import shutil
import subprocess
from .common import LayerTestBase
from cli.test_runner import run_cmd

class TestFimLayerCommit(LayerTestBase):
  """Test suite for fim-layer commit command"""

  def script_exec(self, stdout=None, stderr=None, exit_code=None):
    """Executes a script in the image and checks outputs"""
    out, err, code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "hello-world.sh")
    if stdout:
      self.assertIn(stdout, out)
    if stderr:
      self.assertIn(stderr, err)
    if exit_code:
      self.assertEqual(exit_code, code)
    return (out,err,code)

  def commit(self, content):
    """Commits a layer with a novel script that echoes 'content'"""
    self.create_script(content)
    # Create layer and append to binary
    out,err,code = run_cmd(self.file_image, "fim-layer", "commit", "binary")
    self.assertIn("Filesystem appended to binary", out)
    self.assertEqual(code, 0)
    # Remove directory from host
    shutil.rmtree(self.dir_image, ignore_errors=True)
    # Execute hello-world script which is compressed in the container
    self.script_exec(content, "", 0)
    out,_,code = self.script_exec(content, "", 0)
    os.environ["FIM_DEBUG"] = "1"
    dbg,_,code = self.script_exec(None, "", 0)
    os.environ["FIM_DEBUG"] = "0"
    return (out, dbg.splitlines())

  def test_commit(self):
    """Test committing changes to new layers"""
    count_layers=1
    for i in ["hello world", "second layer", "third layer"]:
      # Check if top-most layer is the one committed
      (output, debug) = self.commit(i)
      self.assertIn(i, output)
      # Count number of overlay layers in debug output
      overlays = set()
      [overlays.add(line) for line in debug if "Overlay layer" in line]
      self.assertEqual(len(overlays), count_layers+1)
      shutil.rmtree(self.dir_image, ignore_errors=True)
      count_layers += 1

  def test_commit_to_file(self):
    """Test committing changes to a separate layer file"""
    # Create script in overlay
    self.create_script("saved to file")
    # Define layer file path
    layer_file = self.dir_image / "tmp" / "test.layer"
    # Commit to file instead of appending to binary
    out, _, code = run_cmd(self.file_image, "fim-layer", "commit", "file", str(layer_file))
    self.assertIn("Layer saved to", out)
    self.assertEqual(code, 0)
    # Remove overlay directory
    shutil.rmtree(self.dir_image / "root", ignore_errors=False)
    # Verify script is NOT in the binary (layer wasn't appended)
    self.script_exec(None, None, 127)
    # Now add the layer file to the binary
    out, _, code = run_cmd(self.file_image, "fim-layer", "add", str(layer_file))
    self.assertEqual(code, 0)
    self.assertIn("Included novel layer from file", out)
    # Remove directory from host again
    shutil.rmtree(self.dir_image / "root", ignore_errors=False)
    # The script should work
    self.script_exec("saved to file", None, 0)
    # Clean up layer file
    if layer_file.exists():
      os.unlink(layer_file)

  def test_commit_to_layer_directory(self):
    """Test committing changes to the managed layers directory"""
    # Create script in overlay
    self.create_script("managed layer")
    # Commit to managed layers directory
    out, err, code = run_cmd(self.file_image, "fim-layer", "commit", "layer")
    self.assertEqual(code, 0)
    self.assertIn("Layer saved to", out)
    # Verify layer file was created in the layers directory
    layers_dir = self.dir_image / "layers"
    # Find the layer file (should be layer-000.layer)
    layer_files = list(layers_dir.glob("layer-*.layer"))
    self.assertEqual(len(layer_files), 1)
    self.assertEqual(layer_files[0].name, "layer-000.layer")
    self.assertGreater(layer_files[0].stat().st_size, 0)

  def test_commit_to_layer_directory_sequence(self):
    """Test layer sequencing"""
    # Create script in overlays
    for i in range(30):
      self.create_script(f"managed layer {i}")
      run_cmd(self.file_image, "fim-layer", "commit", "layer")
    # Find the layer files (should be layer-{000 to 029}.layer)
    layers_dir = self.dir_image / "layers"
    layer_files = sorted(list(layers_dir.glob("layer-*.layer")))
    self.assertEqual(len(layer_files), 30)
    for i in range(30):
      self.assertEqual(layer_files[i].name, "layer-{:03d}.layer".format(i))
      self.assertGreater(layer_files[i].stat().st_size, 0)

  def test_commit_to_layer_directory_exceed_maximum_layers(self):
    """Test the limit of layers"""
    # Create script in overlay
    self.create_script("managed layer fst")
    # Commit to managed layers directory
    out, _, code = run_cmd(self.file_image, "fim-layer", "commit", "layer")
    self.assertIn("Layer saved to", out)
    self.assertEqual(code, 0)
    # Rename the file
    layers_dir = self.dir_image / "layers"
    (layers_dir / "layer-000.layer").rename(layers_dir / "layer-999.layer")
    # Try to create another layer
    self.create_script("managed layer snd")
    _, err, code = run_cmd(self.file_image, "fim-layer", "commit", "layer")
    self.assertIn("Maximum number of layers exceeded", err)
    self.assertEqual(code, 125)

  def test_commit_permissive_file_erasure(self):
    """Test permissive file erasure warnings during commit"""
    # Enable debug mode to capture warning messages
    os.environ["FIM_DEBUG"] = "1"

    try:
      # Create script in overlay
      self.create_script("permissive erasure test")

      # Test case 1: File that cannot be removed
      test_dir = self.dir_image / "root" / "test"
      test_dir.mkdir(parents=True, exist_ok=False)
      test_file = test_dir / "locked_file.txt"
      with open(test_file, "w") as f:
        f.write("This file will be protected")
      # Make the parent directory root-owned so file cannot be removed
      subprocess.run(["sudo", "chown", "root:root", str(test_dir)], check=True)

      # Test case 2: Empty directory that cannot be removed
      # Create a directory that will be empty after its files are removed
      empty_parent = self.dir_image / "root" / "empty_parent"
      empty_dir = empty_parent / "empty_dir"
      empty_dir.mkdir(parents=True, exist_ok=False)
      removable_file = empty_dir / "removable.txt"
      with open(removable_file, "w") as f:
        f.write("This file can be removed")
      # Make only the parent directory root-owned, so:
      # 1. The file CAN be removed (it's user-owned)
      # 2. The directory will be empty after file removal
      # 3. But the directory CANNOT be removed (parent is root-owned)
      subprocess.run(["sudo", "chown", "root:root", str(empty_parent)], check=True)

      # Commit to binary - this should trigger warnings but still succeed
      out, err, code = run_cmd(self.file_image, "fim-layer", "commit", "binary")

      # Check that commit succeeded despite erasure warnings
      self.assertEqual(code, 0)
      self.assertIn("Filesystem appended to binary", out)

      # Check that we reached the "Finished erasing files" line (line 274)
      self.assertIn("Finished erasing files", out)

      # Verify permissive warning messages appear
      self.assertRegex(err, "Could not remove file.*locked_file.txt")
      self.assertRegex(err, "Could not remove directory.*empty_dir")

    finally:
      os.environ["FIM_DEBUG"] = "0"
      # Clean up: restore ownership so tearDown can clean up properly
      subprocess.run(["sudo", "chown", "-R", f"{os.getuid()}:{os.getgid()}", str(self.dir_image / "root")], check=False)