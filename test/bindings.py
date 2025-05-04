#!/bin/python3

import os
import shutil
import subprocess
import unittest
from pathlib import Path

class TestFimBind(unittest.TestCase):

  @classmethod
  def setUpClass(cls):
    cls.dir_script = Path(__file__).resolve().parent
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.dir_image = os.environ["DIR_IMAGE"]

  def setUp(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    )
    return result.stdout.strip()

  def test_add_binding(self):
    output = self.run_cmd("fim-bind", "add", "ro", "/tmp/test/hello/world", "/tmp/test/hello/world")
    self.assertIn("Binding index is '0'", output)

  def test_delete_binding(self):
    self.run_cmd("fim-bind", "add", "ro", "/tmp/test/hello/world", "/tmp/test/hello/world")
    output = self.run_cmd("fim-bind", "del", "0")
    self.assertIn("Erase element with index '0'", output)
    output = self.run_cmd("fim-bind", "list")
    self.assertEqual(output, '{}')

  def test_list_bindings(self):
    self.run_cmd("fim-bind", "add", "ro", "/tmp/test/hello/first", "/tmp/test/hello/entry")
    self.run_cmd("fim-bind", "add", "ro", "/tmp/test/hello/second", "/tmp/test/hello/value")
    output = self.run_cmd("fim-bind", "list")
    bindings = subprocess.run(
      [self.file_image, "fim-bind", "list"],
      stdout=subprocess.PIPE,
      check=True
    )
    import json
    parsed = json.loads(bindings.stdout)
    self.assertEqual(parsed["0"]["src"], "/tmp/test/hello/first")
    self.assertEqual(parsed["0"]["dst"], "/tmp/test/hello/entry")
    self.assertEqual(parsed["1"]["src"], "/tmp/test/hello/second")
    self.assertEqual(parsed["1"]["dst"], "/tmp/test/hello/value")
    self.assertEqual(len(parsed), 2)

  def test_binding_file_contents(self):
    binding_dir = self.dir_script / "binding" / "testing"
    binding_dir.mkdir(parents=True, exist_ok=True)
    test_file = binding_dir / "output"
    test_file.write_text("binding to inside the container")

    self.run_cmd("fim-bind", "add", "ro", str(binding_dir), "/host")
    output = self.run_cmd("fim-exec", "cat", "/host/output")
    self.assertEqual(output.strip(), "binding to inside the container")

    shutil.rmtree(self.dir_script / "binding", ignore_errors=True)

  def test_change_file_contents(self):
    binding_dir = self.dir_script / "binding" / "testing"
    binding_dir.mkdir(parents=True, exist_ok=True)
    test_file = binding_dir / "output"
    test_file.write_text("written by the host")
    # Bind
    self.run_cmd("fim-bind", "add", "rw", str(binding_dir), "/host")
    # Original content
    output = self.run_cmd("fim-exec", "cat", "/host/output")
    self.assertEqual(output.strip(), "written by the host")
    # Writeable file
    output = self.run_cmd("fim-exec", "sh", "-c", "echo 'written by the container' | tee /host/output")
    self.assertEqual(output.strip(), "written by the container")
    # Read-only file
    test_file.write_text("unchanged")
    self.run_cmd("fim-bind", "del", "0")
    self.run_cmd("fim-bind", "add", "ro", str(binding_dir), "/host")
    self.run_cmd("fim-exec", "sh", "-c", "echo 'written by the container' | tee /host/output")
    output = self.run_cmd("fim-exec", "cat", "/host/output")
    self.assertEqual(output.strip(), "unchanged")
    # Cleanup
    shutil.rmtree(self.dir_script / "binding", ignore_errors=True)