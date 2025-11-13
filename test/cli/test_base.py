from genericpath import exists
import unittest
import os
import shutil
from pathlib import Path

class TestBase(unittest.TestCase):
  """
  Base class for desktop integration tests providing shared utilities
  """

  @classmethod
  def setUpClass(cls):
    # Path to the current script
    cls.dir_script = Path(__file__).resolve().parent.parent
    # FlatImage and it's data directories
    cls.file_image = os.environ["FILE_IMAGE"]
    cls.file_image_src = os.environ["FILE_IMAGE_SRC"]
    cls.dir_image = Path(os.environ["DIR_IMAGE"])
    cls.dir_data = Path(os.environ["DIR_DATA"])
    # Desktop integration
    cls.file_desktop = cls.dir_data / "desktop.json"
    cls.dir_xdg = cls.dir_data / "xdg_data_home"

  def setUp(self):
    # Erase data dir
    self.dir_data.mkdir(parents=True, exist_ok=True)
    # Copy fresh image and chmod +x
    shutil.copy(self.file_image_src, self.file_image)
    os.chmod(self.file_image, 0o755)
    # Re-create an empty alternative XDG_DATA_HOME
    shutil.rmtree(self.dir_xdg, ignore_errors=True)
    self.dir_xdg.mkdir(parents=True, exist_ok=False)

  def tearDown(self):
    # Disable debugging
    os.environ["FIM_DEBUG"] = "0"
    # Remove image file an data directory
    os.unlink(self.file_image)
    shutil.rmtree(self.dir_image, ignore_errors=True)
    # Remove custom XDG_DATA_HOME
    shutil.rmtree(self.dir_xdg, ignore_errors=True)
    # Remove desktop integration items
    if self.file_desktop.exists():
      os.unlink(self.file_desktop)
    file_png = self.dir_data / "out.png"
    if file_png.exists():
      os.unlink(file_png)
    file_svg = self.dir_data / "out.svg"
    if file_svg.exists():
      os.unlink(file_svg)
    # Remove temporary image file
    file_image_temp = Path(self.file_image).parent / "temp.flatimage"
    if file_image_temp.exists():
      os.unlink(file_image_temp)
    image_dir = Path(self.file_image).parent / ".temp.flatimage.config"
    if image_dir.exists():
      shutil.rmtree(image_dir)