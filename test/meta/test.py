#!/bin/python3

import os
import sys
import shutil
import unittest
from pathlib import Path

import magic
import build_info

class Suite(unittest.TestSuite):

  def run(self, result, debug=False):
    for test in self:
      self.global_setup()
      if result.shouldStop:
        break
      test(result)
      self.global_teardown()
    return result

  def global_setup(self):
    shutil.copy(os.environ["FILE_IMAGE_SRC"], os.environ["FILE_IMAGE"])

  def global_teardown(self):
    shutil.rmtree(os.environ["DIR_IMAGE"], ignore_errors=True)
    shutil.rmtree(Path(__file__).resolve().parent / "__pycache__", ignore_errors=True)
    os.unlink(os.environ["FILE_IMAGE"])
    

def suite():
  suite = Suite()
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(build_info.TestFimBuildInfo))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(magic.TestFimMagic))
  return suite

if __name__ == '__main__':
  # Resolve script directory
  DIR_SCRIPT = Path(os.path.dirname(__file__))
  # Get input argument
  if len(sys.argv) < 2:
    print("Input image is missing", file=sys.stderr)
    sys.exit(1)
  os.environ["FILE_IMAGE_SRC"] = sys.argv[1]
  os.environ["FILE_IMAGE"] = str(DIR_SCRIPT / "app.flatimage")
  os.environ["DIR_IMAGE"] = str(DIR_SCRIPT / ".app.flatimage.config")
  runner = unittest.TextTestRunner(verbosity=2)
  runner.run(suite())