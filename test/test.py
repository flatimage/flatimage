#!/bin/python3

import os
import sys
import shutil
import unittest
import importlib
from pathlib import Path

# Bindings tests
from cli.bindings.add import TestFimBindAdd
from cli.bindings.list import TestFimBindList
from cli.bindings.delete import TestFimBindDel

# Boot tests
from cli.boot.set import TestFimBootSet
from cli.boot.show import TestFimBootShow
from cli.boot.clear import TestFimBootClear
from cli.boot.cli import TestFimBootCLI

# # Casefold tests
from cli.casefold.on import TestFimCasefoldOn
from cli.casefold.off import TestFimCasefoldOff

# Desktop tests
from cli.desktop.setup import TestFimDesktopSetup
from cli.desktop.enable import TestFimDesktopEnable
from cli.desktop.dump import TestFimDesktopDump
from cli.desktop.clean import TestFimDesktopClean

# Environment tests
from cli.environment.add import TestFimEnvAdd
from cli.environment.delete import TestFimEnvDel
from cli.environment.list import TestFimEnvList
from cli.environment.clear import TestFimEnvClear
from cli.environment.set import TestFimEnvSet
from cli.environment.cli import TestFimEnvCli
from cli.environment.identity import TestFimEnvIdentity

# Exec tests
from cli.execute.execute import TestFimExec

# # Instance tests
from cli.instance.cli import TestFimInstanceCli
from cli.instance.exec import TestFimInstanceExec
from cli.instance.list import TestFimInstanceList

# Layer tests
from cli.layer.commit import TestFimLayerCommit
from cli.layer.create import TestFimLayerCreate

# Overlay tests
from cli.overlay.set import TestFimOverlaySet
from cli.overlay.show import TestFimOverlayShow

# Permissions tests
from cli.permissions.add import TestFimPermsAdd
from cli.permissions.delete import TestFimPermsDel
from cli.permissions.list import TestFimPermsList
from cli.permissions.clear import TestFimPermsClear
from cli.permissions.set import TestFimPermsSet

# Portal tests
from cli.portal.portal import TestFimPortal

# Recipe tests
from cli.recipe.cli import TestFimRecipeCLI
from cli.recipe.fetch import TestFimRecipeFetch
from cli.recipe.info import TestFimRecipeInfo
from cli.recipe.install import TestFimRecipeInstall

# Remote tests
from cli.remote.cli import TestFimRemoteCli
from cli.remote.set import TestFimRemoteSet
from cli.remote.show import TestFimRemoteShow
from cli.remote.clear import TestFimRemoteClear
from cli.remote.workflow import TestFimRemoteWorkflow

# Root tests
from cli.root.root import TestFimRoot

# Version tests
from cli.version.deps import TestFimVersionDeps
from cli.version.full import TestFimVersionFull
from cli.version.short import TestFimVersionShort

def suite():
  suite = unittest.TestSuite()
  # Bindings tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimBindAdd))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimBindList))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimBindDel))
  # Boot tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimBootSet))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimBootShow))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimBootClear))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimBootCLI))
  # Casefold tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimCasefoldOn))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimCasefoldOff))
  # Desktop tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimDesktopSetup))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimDesktopEnable))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimDesktopDump))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimDesktopClean))
  # Environment tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimEnvAdd))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimEnvDel))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimEnvList))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimEnvClear))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimEnvSet))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimEnvCli))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimEnvIdentity))
  # Exec tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimExec))
  # Instance tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimInstanceCli))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimInstanceExec))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimInstanceList))
  # Layer tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimLayerCommit))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimLayerCreate))
  # Overlay tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimOverlaySet))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimOverlayShow))
  # Permissions tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimPermsAdd))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimPermsDel))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimPermsList))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimPermsClear))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimPermsSet))
  # Portal tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimPortal))
  # Recipe tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRecipeCLI))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRecipeFetch))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRecipeInfo))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRecipeInstall))
  # Remote tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRemoteCli))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRemoteSet))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRemoteShow))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRemoteClear))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRemoteWorkflow))
  # Root tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimRoot))
  # Version tests
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimVersionDeps))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimVersionFull))
  suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestFimVersionShort))
  return suite

if __name__ == '__main__':
  # Resolve script directory
  DIR_SCRIPT = Path(os.path.dirname(__file__))
  # Get input argument
  if len(sys.argv) < 2:
    print("Input image is missing", file=sys.stderr)
    sys.exit(1)
  os.environ["FILE_IMAGE_SRC"] = sys.argv[1]
  os.environ["DIR_DATA"] = str(DIR_SCRIPT / "data")
  os.environ["FILE_IMAGE"] = str(DIR_SCRIPT / "data" / "app.flatimage")
  os.environ["DIR_IMAGE"] = str(DIR_SCRIPT / "data" / ".app.flatimage.data")
  runner = unittest.TextTestRunner(verbosity=2)
  runner.run(suite())