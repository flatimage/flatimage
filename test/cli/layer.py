#!/bin/python3

import os
import shutil
import subprocess
import unittest
from pathlib import Path


class TestFimLayer(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = Path(os.environ["DIR_IMAGE"])
    cls.dir_script = Path(__file__).resolve().parent
    cls.file_layer = cls.dir_script / "script.layer"
    cls.dir_root = cls.dir_script / "root"
    cls.dir_bin = cls.dir_root / "usr" / "bin"

  def create_script(self, dir_root, content):
    shutil.rmtree(dir_root, ignore_errors=True)
    dir_bin = dir_root / "usr" / "bin"
    dir_bin.mkdir(parents=True, exist_ok=True)
    hello_script = dir_bin / "hello-world.sh"
    with open(hello_script, "w+") as f:
      f.write('echo "{}"\n'.format(content))
    os.chmod(hello_script, 0o755)
    
  def setUp(self):
    os.environ["FIM_DEBUG"] = "0"
    shutil.rmtree(self.dir_root, ignore_errors=True)
    shutil.copy(os.environ["FILE_IMAGE_SRC"], os.environ["FILE_IMAGE"])

  def tearDown(self):
    os.environ["FIM_DEBUG"] = "0"
    shutil.rmtree(self.dir_root, ignore_errors=True)
    if Path.exists(self.file_layer):
      os.unlink(self.file_layer)

  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)

  def create_and_add_layer(self, content):
    self.create_script(self.dir_root, content)
    # Create layer
    out,err,code = self.run_cmd("fim-layer", "create", str(self.dir_root), str(self.file_layer))
    self.assertIn("filesystem created without errors", err)
    self.assertEqual(code, 0)
    # Add layer
    out,err,code = self.run_cmd("fim-layer", "add", str(self.file_layer))
    self.assertIn("Included novel layer from file", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Remove directory from host
    shutil.rmtree(self.dir_root, ignore_errors=True)
    # Execute hello-world script which is compressed in the container
    out,_,code = self.run_cmd("fim-exec", "sh", "-c", "hello-world.sh")
    self.assertIn(content, out)
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"] = "1"
    debug,_,code = self.run_cmd("fim-exec", "sh", "-c", "hello-world.sh")
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"] = "0"
    return (out, debug.splitlines())


  def test_layer_creation_and_execution(self):
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
    # Missing op
    out,err,code = self.run_cmd("fim-layer")
    self.assertEqual(out, "")
    self.assertIn("Missing op for 'fim-layer' (create,add)", err)
    self.assertEqual(code, 125)
    # Missing source
    out,err,code = self.run_cmd("fim-layer", "create")
    self.assertEqual(out, "")
    self.assertIn("add requires exactly two arguments (/path/to/dir /path/to/file.layer)", err)
    self.assertEqual(code, 125)
    # Missing dest
    out,err,code = self.run_cmd("fim-layer", "create", str(self.dir_root))
    self.assertEqual(out, "")
    self.assertIn("add requires exactly two arguments (/path/to/dir /path/to/file.layer)", err)
    self.assertEqual(code, 125)
    # Source directory does not exist
    out,err,code = self.run_cmd("fim-layer", "create", "/hello/world", str(self.file_layer))
    self.assertIn("Gathering files to compress", out)
    self.assertIn("Source directory '/hello/world' does not exist", err)
    self.assertEqual(code, 125)
    # Source is not a directory
    out,err,code = self.run_cmd("fim-layer", "create", str(Path(__file__).resolve()), str(self.file_layer))
    self.assertIn("Gathering files to compress", out)
    self.assertIn(f"Source '{str(Path(__file__).resolve())}' is not a directory", err)
    self.assertEqual(code, 125)

  def commit(self, content):
    self.create_script(self.dir_image / "overlays" / "upperdir", content)
    # Create layer
    out,err,code = self.run_cmd("fim-layer", "commit")
    self.assertIn("Included novel layer from file", out)
    self.assertEqual(code, 0)
    # Remove directory from host
    shutil.rmtree(self.dir_image, ignore_errors=True)
    # Execute hello-world script which is compressed in the container
    out,err,code = self.run_cmd("fim-exec", "sh", "-c", "hello-world.sh")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"] = "1"
    dbg,err,code = self.run_cmd("fim-exec", "sh", "-c", "hello-world.sh")
    self.assertEqual(code, 0)
    os.environ["FIM_DEBUG"] = "0"
    return (out, dbg.splitlines())

  def test_commit(self):
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