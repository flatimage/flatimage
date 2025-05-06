#!/bin/python3

import os
import shutil
import subprocess
import unittest
from pathlib import Path


class TestFimCommit(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = Path(os.environ["DIR_IMAGE"])
    cls.dir_script = Path(__file__).resolve().parent
    cls.dir_bin = cls.dir_image / "overlays" / "upperdir" / "usr" / "bin"

  def create_script(self, content):
    shutil.rmtree(self.dir_image, ignore_errors=True)
    (self.dir_bin).mkdir(parents=True, exist_ok=True)
    hello_script = self.dir_bin / "hello-world.sh"
    with open(hello_script, "w") as f:
      f.write('echo "{}"\n'.format(content))
    os.chmod(hello_script, 0o755)
    
  def setUp(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def tearDown(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    )
    return result.stdout.strip()

  def commit(self, content):
    self.create_script(content)
    # Create layer
    output = self.run_cmd("fim-commit")
    self.assertIn("Included novel layer from file", output)
    # Remove directory from host
    shutil.rmtree(self.dir_image, ignore_errors=True)
    # Execute hello-world script which is compressed in the container
    output = self.run_cmd("fim-exec", "sh", "-c", "hello-world.sh")
    os.environ["FIM_DEBUG"] = "1"
    debug = self.run_cmd("fim-exec", "sh", "-c", "hello-world.sh")
    del os.environ["FIM_DEBUG"]
    return (output, debug.splitlines())


  def test_commit(self):
    count_layers=1
    for i in ["hello world", "second layer", "third layer"]:
      (output, debug) = self.commit(i)
      self.assertIn(i, output)
      count = sum(1 for line in debug if "Overlay layer" in line)
      self.assertEqual(count, count_layers+1)
      shutil.rmtree(self.dir_image, ignore_errors=True)
      count_layers += 1