#!/bin/python3

import os
import shutil
from pathlib import Path
from .common import LayerTestBase
from cli.test_runner import run_cmd

class TestFimLayerCreate(LayerTestBase):
  """Test suite for fim-layer create and add commands"""

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.file_layer = cls.dir_data / "script.layer"
    cls.dir_bin = cls.dir_root / "usr" / "bin"

  def tearDown(self):
    super().tearDown()
    if Path.exists(self.file_layer):
      os.unlink(self.file_layer)

  def create_and_add_layer(self, content):
    self.create_script(content, self.dir_root)
    # Create layer
    out,err,code = run_cmd(self.file_image, "fim-layer", "create", str(self.dir_root), str(self.file_layer))
    self.assertIn("Filesystem created without errors", out)
    self.assertEqual(code, 0)
    # Add layer
    out,err,code = run_cmd(self.file_image, "fim-layer", "add", str(self.file_layer))
    self.assertIn("Included novel layer from file", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Remove directory from host
    shutil.rmtree(self.dir_root, ignore_errors=True)
    # Execute hello-world script which is compressed in the container
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "hello-world.sh")
    self.assertIn(content, out)
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"] = "1"
    debug,_,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "hello-world.sh")
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"] = "0"
    return (out, debug.splitlines())

  def test_layer_creation_and_execution(self):
    """Test creating and adding multiple layers"""
    count_layers=1
    for i in ["hello world", "second layer", "third layer"]:
      (output, debug) = self.create_and_add_layer(i)
      self.assertIn(i, output)
      overlays = set()
      [overlays.add(line) for line in debug if "Overlay layer" in line]
      self.assertEqual(len(overlays), count_layers+1)
      shutil.rmtree(self.dir_root, ignore_errors=True)
      count_layers += 1

  def test_layer_cli(self):
    """Test CLI argument validation for create command"""
    # Missing op
    out,err,code = run_cmd(self.file_image, "fim-layer")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-layer' (create,add,commit)", err)
    self.assertEqual(code, 125)
    # Missing source
    out,err,code = run_cmd(self.file_image, "fim-layer", "create")
    self.assertEqual(out, "")
    self.assertIn("create requires exactly two arguments (/path/to/dir /path/to/file.layer)", err)
    self.assertEqual(code, 125)
    # Missing dest
    out,err,code = run_cmd(self.file_image, "fim-layer", "create", str(self.dir_root))
    self.assertEqual(out, "")
    self.assertIn("create requires exactly two arguments (/path/to/dir /path/to/file.layer)", err)
    self.assertEqual(code, 125)
    # Source directory does not exist
    out,err,code = run_cmd(self.file_image, "fim-layer", "create", "/hello/world", str(self.file_layer))
    self.assertIn("Gathering files to compress", out)
    self.assertIn("Source directory '/hello/world' does not exist", err)
    self.assertEqual(code, 125)
    # Source is not a directory
    out,err,code = run_cmd(self.file_image, "fim-layer", "create", str(Path(__file__).resolve()), str(self.file_layer))
    self.assertIn("Gathering files to compress", out)
    self.assertIn(f"Source '{str(Path(__file__).resolve())}' is not a directory", err)
    self.assertEqual(code, 125)
