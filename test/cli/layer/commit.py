#!/bin/python3

import os
import shutil
from .common import LayerTestBase
from cli.test_runner import run_cmd

class TestFimLayerCommit(LayerTestBase):
  """Test suite for fim-layer commit command"""

  def commit(self, content):
    self.create_script(self.dir_image / "data", content)
    # Create layer
    out,err,code = run_cmd(self.file_image, "fim-layer", "commit")
    self.assertIn("Included novel layer from file", out)
    self.assertEqual(code, 0)
    # Remove directory from host
    shutil.rmtree(self.dir_image, ignore_errors=True)
    # Execute hello-world script which is compressed in the container
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "hello-world.sh")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"] = "1"
    dbg,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "hello-world.sh")
    self.assertEqual(code, 0)
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
