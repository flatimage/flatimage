#!/bin/python3

import os
import sys
import shutil
import unittest
from pathlib import Path

import bindings
import boot
import commit
import desktop
import environment
import exec
import layer
import metadata
import permissions
import root
import magic
import portal
import instance

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
    shutil.copy(os.environ["FILE_IMAGE_SRC"], os.environ["FILE_IMAGE"])
    

def suite():
  suite = Suite()
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(metadata.TestFimMetadata))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(bindings.TestFimBind))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(boot.TestFimBoot))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(commit.TestFimCommit))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(desktop.TestFimDesktop))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(environment.TestFimEnv))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(exec.TestFimExec))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(layer.TestFimLayer))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(permissions.TestFimPerms))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(root.TestFimRoot))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(magic.TestFimMagic))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(portal.TestFimPortal))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(instance.TestFimInstance))
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
  shutil.rmtree(os.environ["FILE_IMAGE"], ignore_errors=True)