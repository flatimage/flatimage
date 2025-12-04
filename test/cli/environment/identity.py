#!/bin/python3
"""
Test suite for fim-env user identity configuration.
"""

import os
import getpass
import pwd
from .common import EnvTestBase
from cli.test_runner import run_cmd

class TestFimEnvIdentity(EnvTestBase):
  """
  Tests for user identity configuration (UID, GID, USER, HOME, SHELL, PS1).
  """

  # === User Identity Configuration Tests ===

  def test_custom_uid_gid(self):
    """Test custom UID and GID configuration"""
    # Set custom UID and GID
    out,err,code = run_cmd(self.file_image, "fim-env", "add", "UID=5000", "GID=5000")
    self.assertIn("Included variable 'UID' with value '5000'", out)
    self.assertIn("Included variable 'GID' with value '5000'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify UID in container
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(out, "5000")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify GID in container
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-g")
    self.assertEqual(out, "5000")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify full id output
    out,err,code = run_cmd(self.file_image, "fim-exec", "id")
    self.assertIn("uid=5000", out)
    self.assertIn("gid=5000", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_custom_user(self):
    """Test custom USER configuration"""
    # Set custom username
    out,err,code = run_cmd(self.file_image, "fim-env", "add", "USER=appuser")
    self.assertIn("Included variable 'USER' with value 'appuser'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify username with whoami
    out,err,code = run_cmd(self.file_image, "fim-exec", "whoami")
    self.assertEqual(out, "appuser")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify passwd file contains custom username
    out,err,code = run_cmd(self.file_image, "fim-exec", "cat", "/etc/passwd")
    self.assertIn("appuser:x:", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_custom_home(self):
    """Test custom HOME directory configuration"""
    # Set custom home directory
    out,err,code = run_cmd(self.file_image, "fim-env", "add", "HOME=/opt/app/home")
    self.assertIn("Included variable 'HOME' with value '/opt/app/home'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify HOME environment variable
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo $HOME")
    self.assertEqual(out, "/opt/app/home")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify passwd file contains custom home
    out,err,code = run_cmd(self.file_image, "fim-exec", "cat", "/etc/passwd")
    self.assertIn(":/opt/app/home:", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_custom_shell(self):
    """Test custom SHELL configuration"""
    # Set custom shell
    out,err,code = run_cmd(self.file_image, "fim-env", "add", "SHELL=/bin/sh")
    self.assertIn("Included variable 'SHELL' with value '/bin/sh'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify passwd file contains custom shell
    out,err,code = run_cmd(self.file_image, "fim-exec", "cat", "/etc/passwd")
    self.assertIn(":/bin/sh", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)

  def test_custom_ps1(self):
    """Test custom PS1 prompt configuration"""
    # Set custom prompt
    custom_prompt = "[myapp] \\W > "
    out,err,code = run_cmd(self.file_image, "fim-env", "add", f"PS1={custom_prompt}")
    self.assertIn(f"Included variable 'PS1' with value '{custom_prompt}'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify PS1 in bashrc
    out,_,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "cat $BASHRC_FILE 2>/dev/null")
    # The bashrc should contain the custom PS1
    self.assertIn(custom_prompt, out)
    self.assertEqual(code, 0)

  def test_complete_custom_identity(self):
    """Test complete custom identity with all variables"""
    # Set all identity variables at once
    out,err,code = run_cmd(self.file_image, "fim-env", "add",
      "UID=5000",
      "GID=5000",
      "USER=webapp",
      "HOME=/app",
      "SHELL=/bin/sh")
    self.assertIn("Included variable 'UID' with value '5000'", out)
    self.assertIn("Included variable 'GID' with value '5000'", out)
    self.assertIn("Included variable 'USER' with value 'webapp'", out)
    self.assertIn("Included variable 'HOME' with value '/app'", out)
    self.assertIn("Included variable 'SHELL' with value '/bin/sh'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify UID
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(out, "5000")
    self.assertEqual(code, 0)
    # Verify GID
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-g")
    self.assertEqual(out, "5000")
    self.assertEqual(code, 0)
    # Verify username
    out,err,code = run_cmd(self.file_image, "fim-exec", "whoami")
    self.assertEqual(out, "webapp")
    self.assertEqual(code, 0)
    # Verify HOME
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo $HOME")
    self.assertEqual(out, "/app")
    self.assertEqual(code, 0)
    # Verify complete passwd entry
    out,err,code = run_cmd(self.file_image, "fim-exec", "cat", "/etc/passwd")
    self.assertIn("webapp:x:5000:5000:webapp:/app:/bin/sh", out)
    self.assertEqual(code, 0)

  def test_root_identity_with_fim_root(self):
    """Test that FIM_ROOT automatically configures root identity"""
    # Verify UID is 0
    os.environ["FIM_ROOT"] = "1"
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(out, "0")
    self.assertEqual(code, 0)
    # Verify username is automatically "root"
    out,err,code = run_cmd(self.file_image, "fim-exec", "whoami")
    self.assertEqual(out, "root")
    self.assertEqual(code, 0)
    # home should be un-changed
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo $HOME")
    self.assertEqual(out, "/root")
    self.assertEqual(code, 0)
    # Verify passwd entry for root
    out,err,code = run_cmd(self.file_image, "fim-exec", "cat", "/etc/passwd")
    self.assertIn("root:x:0:0:root:/root:", out)
    self.assertEqual(code, 0)
    os.environ["FIM_ROOT"] = "0"

  def test_root_identity_with_uid_zero(self):
    """Test that UID=0 automatically configures root identity"""
    # Set UID to 0 (should auto-configure as root)
    out,err,code = run_cmd(self.file_image, "fim-env", "add", "UID=0", "GID=0")
    self.assertIn("Included variable 'UID' with value '0'", out)
    self.assertIn("Included variable 'GID' with value '0'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify UID is 0
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(out, "0")
    self.assertEqual(code, 0)
    # Verify username is automatically "root"
    out,err,code = run_cmd(self.file_image, "fim-exec", "whoami")
    self.assertEqual(out, "root")
    self.assertEqual(code, 0)
    # home should be un-changed
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo $HOME")
    self.assertEqual(out, "/root")
    self.assertEqual(code, 0)
    # Verify passwd entry for root
    out,err,code = run_cmd(self.file_image, "fim-exec", "cat", "/etc/passwd")
    self.assertIn("root:x:0:0:root:/root:", out)
    self.assertEqual(code, 0)

  def test_root_mode_override(self):
    """Test that fim-root always uses UID=0 regardless of custom values"""
    # Set custom UID/GID
    out,err,code = run_cmd(self.file_image, "fim-env", "add", "UID=5000", "GID=5000")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # fim-exec should use custom UID
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(out, "5000")
    self.assertEqual(code, 0)
    # fim-root should override and use UID=0
    out,err,code = run_cmd(self.file_image, "fim-root", "id", "-u")
    self.assertEqual(out, "0")
    self.assertEqual(code, 0)
    # fim-root should be root user
    out,err,code = run_cmd(self.file_image, "fim-root", "whoami")
    self.assertEqual(out, "root")
    self.assertEqual(code, 0)
    # Verify passwd entry for root
    out,err,code = run_cmd(self.file_image, "fim-root", "cat", "/etc/passwd")
    self.assertIn("root:x:0:0:root:/root:", out)
    self.assertEqual(code, 0)

  def test_invalid_uid_fallback(self):
    """Test that invalid UID values fall back to host defaults"""
    # Set invalid UID (non-numeric)
    out,err,code = run_cmd(self.file_image, "fim-env", "add", "UID=notanumber")
    self.assertIn("Included variable 'UID' with value 'notanumber'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Should fall back to host UID (not 0 or error)
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(int(out), os.getuid())
    self.assertEqual(code, 0)

  def test_invalid_gid_fallback(self):
    """Test that invalid GID values fall back to host defaults"""
    # Set invalid GID (non-numeric)
    out,err,code = run_cmd(self.file_image, "fim-env", "add", "GID=notanumber")
    self.assertIn("Included variable 'GID' with value 'notanumber'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Should fall back to host GID (not 0 or error)
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-g")
    self.assertEqual(int(out), os.getgid())
    self.assertEqual(code, 0)

  def test_user_identity_reset(self):
    """Test resetting user identity to defaults"""
    # Set custom identity
    out,err,code = run_cmd(self.file_image, "fim-env", "add",
      "UID=5000",
      "GID=5000",
      "USER=webapp",
      "HOME=/app")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify custom values work
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(out, "5000")
    # Reset by deleting all identity variables
    out,err,code = run_cmd(self.file_image, "fim-env", "del", "UID", "GID", "USER", "HOME")
    self.assertIn("Erase key 'UID'", out)
    self.assertIn("Erase key 'GID'", out)
    self.assertIn("Erase key 'USER'", out)
    self.assertIn("Erase key 'HOME'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify returns to host defaults (UID should not be 5000)
    out,err,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(int(out), os.getgid())
    self.assertEqual(code, 0)

  def test_default_behavior_no_custom_values(self):
    """Test default behavior when no identity variables are set"""
    # Should have some UID (host UID)
    out,_,code = run_cmd(self.file_image, "fim-exec", "id", "-u")
    self.assertEqual(int(out), os.getuid())
    self.assertEqual(code, 0)
    # Should have some GID (host GID)
    out,_,code = run_cmd(self.file_image, "fim-exec", "id", "-g")
    self.assertEqual(int(out), os.getgid())
    self.assertEqual(code, 0)
    # Should have a username (host username)
    out,_,code = run_cmd(self.file_image, "fim-exec", "whoami")
    self.assertNotEqual(out, "")
    self.assertEqual(code, 0)
    # Should have HOME set
    out,_,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "echo $HOME")
    self.assertEqual(out, os.environ["HOME"])
    self.assertEqual(code, 0)

  def test_passwd_file_generation_custom(self):
    """Test that /etc/passwd is properly generated with user info"""
    # Set custom identity
    out,err,code = run_cmd(self.file_image, "fim-env", "add",
      "UID=4500",
      "GID=4500",
      "HOME=/home/custom",
      "USER=testuser",
      "SHELL=/bin/sh")
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify passwd file exists and is readable
    out,_,code = run_cmd(self.file_image, "fim-exec", "test", "-r", "/etc/passwd")
    self.assertEqual(code, 0)
    # Verify passwd file contains our entry
    out,_,code = run_cmd(self.file_image, "fim-exec", "cat", "/etc/passwd")
    self.assertEqual(code, 0)
    self.assertIn("testuser:x:4500:4500:", out)
    # Verify passwd format is correct (7 fields separated by colons)
    passwd_line = out.strip()
    fields = passwd_line.split(':')
    self.assertEqual(len(fields), 7)
    self.assertEqual(fields[0], "testuser")       # username
    self.assertEqual(fields[1], "x")              # password placeholder
    self.assertEqual(fields[2], "4500")           # UID
    self.assertEqual(fields[3], "4500")           # GID
    self.assertEqual(fields[4], "testuser")       # GECOS
    self.assertEqual(fields[5], "/home/custom")   # HOME
    self.assertRegex(fields[6], "/bin/sh")        # SHELL

  def test_passwd_file_generation_default(self):
    """Test that /etc/passwd is properly generated with default user info"""
    # Verify passwd file exists and is readable
    out,_,code = run_cmd(self.file_image, "fim-exec", "test", "-r", "/etc/passwd")
    self.assertEqual(code, 0)
    # Verify passwd file contains our entry
    out,_,code = run_cmd(self.file_image, "fim-exec", "cat", "/etc/passwd")
    self.assertEqual(code, 0)
    # Verify passwd format is correct (7 fields separated by colons)
    passwd_line = out.strip()
    fields = passwd_line.split(':')
    user_info = pwd.getpwnam(getpass.getuser())
    self.assertEqual(len(fields), 7)
    self.assertEqual(fields[0], user_info.pw_name)       # username
    self.assertEqual(fields[1], "x")                     # password placeholder
    self.assertEqual(fields[2], str(user_info.pw_uid))   # UID
    self.assertEqual(fields[3], str(user_info.pw_gid))   # GID
    # [4] GECOS
    self.assertEqual(fields[5], user_info.pw_dir)        # HOME
    self.assertRegex(fields[6], "/tmp/fim/app.*/bin/bash")   # SHELL

  def test_combined_user_and_ps1(self):
    """Test that USER and PS1 work together"""
    # Set both USER and custom PS1
    out,err,code = run_cmd(self.file_image, "fim-env", "add",
      "USER=myapp",
      "PS1=[\\u] \\W > ")
    self.assertIn("Included variable 'USER' with value 'myapp'", out)
    self.assertIn("Included variable 'PS1'", out)
    self.assertEqual(err, "")
    self.assertEqual(code, 0)
    # Verify username
    out,err,code = run_cmd(self.file_image, "fim-exec", "whoami")
    self.assertEqual(out, "myapp")
    self.assertEqual(code, 0)
    # PS1 should be in bashrc with \\u that will expand to myapp
    out,err,code = run_cmd(self.file_image, "fim-exec", "sh", "-c", "cat $BASHRC_FILE")
    self.assertIn("[\\u] \\W >", out)
    self.assertEqual(code, 0)
