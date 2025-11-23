#!/bin/python3

import os
import shutil
import unittest
from pathlib import Path
from cli.test_runner import run_cmd

class TestFimDirData(unittest.TestCase):
  """
  Tests for FIM_DIR_DATA feature - multiple data directories for the same binary.

  The FIM_DIR_DATA environment variable allows different instances to use separate
  data directories (upper layers in the overlay filesystem), enabling isolated
  runtime storage for different configurations of the same application binary.
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

    # Create two separate data directories for testing
    cls.dir_data_1 = cls.dir_data / "data_dir_1"
    cls.dir_data_2 = cls.dir_data / "data_dir_2"

  def setUp(self):
    # Ensure data directories exist
    self.dir_data.mkdir(parents=True, exist_ok=True)
    # Copy fresh image and chmod +x
    shutil.copy(self.file_image_src, self.file_image)
    os.chmod(self.file_image, 0o755)

  def tearDown(self):
    # Remove image file
    if Path(self.file_image).exists():
      os.unlink(self.file_image)
    # Remove data directories
    shutil.rmtree(self.dir_data_1, ignore_errors=True)
    shutil.rmtree(self.dir_data_2, ignore_errors=True)
    # Remove default data directory
    shutil.rmtree(self.dir_image, ignore_errors=True)

  def test_separate_writable_layers(self):
    """
    Test that two instances with different FIM_DIR_DATA values maintain separate writable layers.

    This test:
    1. Writes a file to /instance_1/test_file.txt in instance 1
    2. Writes a different file to /instance_2/test_file.txt in instance 2
    3. Verifies each instance reads back its own content
    """
    # Environment for instance 1
    env1 = os.environ.copy()
    env1["FIM_DIR_DATA"] = str(self.dir_data_1)

    # Environment for instance 2
    env2 = os.environ.copy()
    env2["FIM_DIR_DATA"] = str(self.dir_data_2)

    # Write file in instance 1
    out1, err1, code1 = run_cmd(
      self.file_image,
      "fim-exec", "sh", "-c", "mkdir -p /instance_1 && echo 'data_from_instance_1' > /instance_1/test_file.txt",
      env=env1
    )
    self.assertEqual(code1, 0, f"Failed to write file in instance 1: {err1}")

    # Write different file in instance 2
    out2, err2, code2 = run_cmd(
      self.file_image,
      "fim-exec", "sh", "-c", "mkdir -p /instance_2 && echo 'data_from_instance_2' > /instance_2/test_file.txt",
      env=env2
    )
    self.assertEqual(code2, 0, f"Failed to write file in instance 2: {err2}")

    # Read from instance 1 - should get instance 1 data
    out1, err1, code1 = run_cmd(
      self.file_image,
      "fim-exec", "cat", "/instance_1/test_file.txt",
      env=env1
    )
    self.assertEqual(code1, 0, f"Failed to read file in instance 1: {err1}")
    self.assertIn("data_from_instance_1", out1)
    self.assertNotIn("data_from_instance_2", out1)

    # Verify instance 2 file doesn't exist in instance 1
    out1, err1, code1 = run_cmd(
      self.file_image,
      "fim-exec", "test", "-f", "/instance_2/test_file.txt",
      env=env1
    )
    self.assertNotEqual(code1, 0, "Instance 2 file should not exist in instance 1")

    # Read from instance 2 - should get instance 2 data
    out2, err2, code2 = run_cmd(
      self.file_image,
      "fim-exec", "cat", "/instance_2/test_file.txt",
      env=env2
    )
    self.assertEqual(code2, 0, f"Failed to read file in instance 2: {err2}")
    self.assertIn("data_from_instance_2", out2)
    self.assertNotIn("data_from_instance_1", out2)

    # Verify instance 1 file doesn't exist in instance 2
    out2, err2, code2 = run_cmd(
      self.file_image,
      "fim-exec", "test", "-f", "/instance_1/test_file.txt",
      env=env2
    )
    self.assertNotEqual(code2, 0, "Instance 1 file should not exist in instance 2")

  def test_separate_directory_structures(self):
    """
    Test that directory structures are isolated between instances.

    This test:
    1. Creates different directory structures in each instance
    2. Verifies each instance sees only its own directories
    """
    # Environment for instance 1
    env1 = os.environ.copy()
    env1["FIM_DIR_DATA"] = str(self.dir_data_1)

    # Environment for instance 2
    env2 = os.environ.copy()
    env2["FIM_DIR_DATA"] = str(self.dir_data_2)

    # Create directory structure in instance 1
    out1, err1, code1 = run_cmd(
      self.file_image,
      "fim-exec", "sh", "-c",
      "mkdir -p /app_data/config && echo 'config1' > /app_data/config/settings.conf",
      env=env1
    )
    self.assertEqual(code1, 0, f"Failed to create directory in instance 1: {err1}")

    # Create different directory structure in instance 2
    out2, err2, code2 = run_cmd(
      self.file_image,
      "fim-exec", "sh", "-c",
      "mkdir -p /app_data/config && echo 'config2' > /app_data/config/settings.conf",
      env=env2
    )
    self.assertEqual(code2, 0, f"Failed to create directory in instance 2: {err2}")

    # Read config from instance 1
    out1, err1, code1 = run_cmd(
      self.file_image,
      "fim-exec", "cat", "/app_data/config/settings.conf",
      env=env1
    )
    self.assertEqual(code1, 0, f"Failed to read config in instance 1: {err1}")
    self.assertIn("config1", out1)
    self.assertNotIn("config2", out1)

    # Read config from instance 2
    out2, err2, code2 = run_cmd(
      self.file_image,
      "fim-exec", "cat", "/app_data/config/settings.conf",
      env=env2
    )
    self.assertEqual(code2, 0, f"Failed to read config in instance 2: {err2}")
    self.assertIn("config2", out2)
    self.assertNotIn("config1", out2)

  def test_default_data_directory_when_unset(self):
    """
    Test that when FIM_DIR_DATA is not set, the default directory is used.

    The default directory should be next to the binary as .{binary_name}.data
    """
    # Don't set FIM_DIR_DATA, use default behavior
    env = os.environ.copy()
    if "FIM_DIR_DATA" in env:
      del env["FIM_DIR_DATA"]

    # Write a file
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "sh", "-c", "mkdir -p /default_test && echo 'default_instance_data' > /default_test/file.txt",
      env=env
    )
    self.assertEqual(code, 0, f"Failed to write file with default dir: {err}")

    # Verify the default data directory was created
    default_data_dir = Path(self.file_image).parent / f".{Path(self.file_image).name}.data"
    self.assertTrue(default_data_dir.exists(),
                    f"Default data directory should exist at {default_data_dir}")

    # Verify the upper layer (data/) exists
    upper_dir = default_data_dir / "data"
    self.assertTrue(upper_dir.exists(),
                    f"Upper layer directory should exist at {upper_dir}")

    # Read back the file
    out, err, code = run_cmd(
      self.file_image,
      "fim-exec", "cat", "/default_test/file.txt",
      env=env
    )
    self.assertEqual(code, 0, f"Failed to read file with default dir: {err}")
    self.assertIn("default_instance_data", out)

  def test_physical_data_isolation(self):
    """
    Test that the physical data directories on the host are actually separate.

    This test verifies that files written to the overlay in the container end up in
    the correct host_data/data/ directory.
    """
    # Environment for instance 1
    env1 = os.environ.copy()
    env1["FIM_DIR_DATA"] = str(self.dir_data_1)

    # Environment for instance 2
    env2 = os.environ.copy()
    env2["FIM_DIR_DATA"] = str(self.dir_data_2)

    # Write file in instance 1
    out1, err1, code1 = run_cmd(
      self.file_image,
      "fim-exec", "sh", "-c", "mkdir -p /host_check && echo 'instance1_data' > /host_check/isolation_test.txt",
      env=env1
    )
    self.assertEqual(code1, 0, f"Failed to write file in instance 1: {err1}")

    # Write file in instance 2
    out2, err2, code2 = run_cmd(
      self.file_image,
      "fim-exec", "sh", "-c", "mkdir -p /host_check && echo 'instance2_data' > /host_check/isolation_test.txt",
      env=env2
    )
    self.assertEqual(code2, 0, f"Failed to write file in instance 2: {err2}")

    # Check physical host directories
    upper_dir_1 = self.dir_data_1 / "data" / "host_check"
    upper_dir_2 = self.dir_data_2 / "data" / "host_check"

    # Both should exist
    self.assertTrue(upper_dir_1.exists(),
                   f"Instance 1 upper layer should exist at {upper_dir_1}")
    self.assertTrue(upper_dir_2.exists(),
                   f"Instance 2 upper layer should exist at {upper_dir_2}")

    # Check files exist in the correct locations
    file_1 = upper_dir_1 / "isolation_test.txt"
    file_2 = upper_dir_2 / "isolation_test.txt"

    self.assertTrue(file_1.exists(),
                   f"File should exist in instance 1 upper layer at {file_1}")
    self.assertTrue(file_2.exists(),
                   f"File should exist in instance 2 upper layer at {file_2}")

    # Verify content is different
    with open(file_1, 'r') as f:
      content_1 = f.read()
    with open(file_2, 'r') as f:
      content_2 = f.read()

    self.assertIn("instance1_data", content_1)
    self.assertNotIn("instance2_data", content_1)
    self.assertIn("instance2_data", content_2)
    self.assertNotIn("instance1_data", content_2)
