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
    cls.binding_dir = cls.dir_script / "tmp_bind"

  def setUp(self):
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def tearDown(self):
    shutil.rmtree(self.binding_dir, ignore_errors=True)

  def run_cmd(self, *args):
    result = subprocess.run(
      [self.file_image] + list(args),
      stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT,
      text=True
    )
    return result.stdout.strip()
  
  def create_tmp(self, name):
    self.binding_dir.mkdir(parents=True, exist_ok=True)
    test_file = self.binding_dir / name
    if test_file.exists():
      test_file.unlink()
    return test_file

  def test_add_binding(self):
    # Add binding and read file contents
    test_file: Path = self.create_tmp("output")
    output = self.run_cmd("fim-bind", "add", "ro", test_file, "/host/test_file")
    self.assertIn("Binding index is '0'", output)
    test_file.write_text("binding to inside the container")
    output = self.run_cmd("fim-exec", "cat", "/host/test_file")
    self.assertIn("binding to inside the container", output)
    # No option
    output = self.run_cmd("fim-bind", "add")
    self.assertIn("Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)", output)
    # Invalid option
    output = self.run_cmd("fim-bind", "add", "roo")
    self.assertIn("Could not determine enum entry from 'ROO'", output)
    # No source
    output = self.run_cmd("fim-bind", "add", "ro")
    self.assertIn("Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)", output)
    # No dst
    output = self.run_cmd("fim-bind", "add", "ro", "/tmp/test/hello/world")
    self.assertIn("Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)", output)
    # Extra args
    output = self.run_cmd("fim-bind", "add", "ro", test_file, "/host/test_file", "heya")
    self.assertIn("Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>", output)

  def test_delete_binding(self):
    # Add binding and read file contents
    test_file: Path = self.create_tmp("output")
    output = self.run_cmd("fim-bind", "add", "ro", test_file, "/host/test_file")
    self.assertIn("Binding index is '0'", output)
    test_file.write_text("binding to inside the container")
    output = self.run_cmd("fim-exec", "cat", "/host/test_file")
    self.assertIn("binding to inside the container", output)
    # Check if deletion works
    output = self.run_cmd("fim-bind", "del", "0")
    self.assertIn("Erase element with index '0'", output)
    output = self.run_cmd("fim-bind", "list")
    self.assertEqual(output, '')
    output = self.run_cmd("fim-exec", "cat", "/host/test_file")
    self.assertEqual(output, '')
    # Invalid index
    output = self.run_cmd("fim-bind", "del", "0")
    self.assertIn("No element with index '0' found", output)
    output = self.run_cmd("fim-bind", "del", "aaa")
    self.assertIn("Index argument for 'del' is not a number", output)
    # Extra arguments
    output = self.run_cmd("fim-bind", "add", "ro", test_file, "/host/test_file")
    self.assertIn("Binding index is '0'", output)
    output = self.run_cmd("fim-bind", "del", "0", "1")
    self.assertIn("Incorrect number of arguments for 'del' (<index>)", output)

  def test_list_bindings(self):
    test_file_1: Path = self.create_tmp("output1")
    test_file_2: Path = self.create_tmp("output2")
    self.run_cmd("fim-bind", "add", "ro", test_file_1, "/host/files/file_1")
    self.run_cmd("fim-bind", "add", "ro", test_file_2, "/host/files/file_2")
    output = self.run_cmd("fim-bind", "list")
    # Check json output
    import json
    parsed = json.loads(output)
    self.assertEqual(parsed["0"]["src"], str(test_file_1))
    self.assertEqual(parsed["0"]["dst"], "/host/files/file_1")
    self.assertEqual(parsed["1"]["src"], str(test_file_2))
    self.assertEqual(parsed["1"]["dst"], "/host/files/file_2")
    self.assertEqual(len(parsed), 2)
    # Extra arguments
    output = self.run_cmd("fim-bind", "list", "0")
    self.assertIn("'list' command takes no arguments", output)

    shutil.rmtree(self.dir_script / "binding", ignore_errors=True)

  def test_change_file_contents(self):
    test_file: Path = self.create_tmp("output")
    test_file.write_text("written by the host")
    # Bind
    self.run_cmd("fim-bind", "add", "rw", test_file, "/host/output")
    # Original content
    output = self.run_cmd("fim-exec", "cat", "/host/output")
    self.assertEqual(output.strip(), "written by the host")
    # Writeable file
    self.run_cmd("fim-exec", "sh", "-c", "echo 'written by the container' > /host/output")
    output = self.run_cmd("fim-exec", "cat", "/host/output")
    self.assertEqual(output.strip(), "written by the container")

  def test_readonly_file(self):
    test_file: Path = self.create_tmp("output")
    test_file.write_text("unchanged")
    self.run_cmd("fim-bind", "del", "0")
    self.run_cmd("fim-bind", "add", "ro", test_file, "/host/output")
    self.run_cmd("fim-exec", "sh", "-c", "echo 'written by the container' > /host/output")
    output = self.run_cmd("fim-exec", "cat", "/host/output")
    self.assertEqual(output.strip(), "unchanged")