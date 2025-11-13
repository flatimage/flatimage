#!/bin/python3
"""
Common utilities for FlatImage CLI tests.

This module provides shared functionality across all test modules,
including command execution helpers and common test patterns.
"""

import os
import subprocess
from typing import Tuple, Optional, Dict


def run_cmd(file_image: str, *args, env: Optional[Dict[str, str]] = None) -> Tuple[str, str, int]:
    """
    Execute a command against the FlatImage binary.
    
    Args:
        file_image: Path to the FlatImage binary
        *args: Command arguments to pass to the binary
        env: Optional environment variables (defaults to os.environ.copy())
    
    Returns:
        Tuple of (stdout, stderr, returncode) where stdout and stderr are stripped
    
    Example:
        >>> out, err, code = run_cmd("/path/to/app.flatimage", "fim-version")
        >>> print(out)
    """
    result = subprocess.run(
        [file_image] + list(args),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=env or os.environ.copy()
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)


def run_cmd_with_stdin(file_image: str, *args, env: Optional[Dict[str, str]] = None) -> Tuple[str, str, int]:
    """
    Execute a command against the FlatImage binary with stdin pipe available.
    
    Args:
        file_image: Path to the FlatImage binary
        *args: Command arguments to pass to the binary
        env: Optional environment variables (defaults to os.environ.copy())
    
    Returns:
        Tuple of (stdout, stderr, returncode) where stdout and stderr are stripped
    
    Example:
        >>> out, err, code = run_cmd_with_stdin("/path/to/app.flatimage", "fim-instance", "list")
        >>> print(out)
    """
    result = subprocess.run(
        [file_image] + list(args),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
        env=env or os.environ.copy(),
        text=True
    )
    return (result.stdout.strip(), result.stderr.strip(), result.returncode)


def spawn_cmd(file_image: str, *args) -> subprocess.Popen:
    """
    Spawn a background command against the FlatImage binary.
    
    Args:
        file_image: Path to the FlatImage binary
        *args: Command arguments to pass to the binary
    
    Returns:
        Popen process object
    
    Example:
        >>> proc = spawn_cmd("/path/to/app.flatimage", "fim-root", "sleep", "10")
        >>> # ... do other work ...
        >>> proc.kill()
    """
    return subprocess.Popen(
        ["bash", "-c", f"{file_image} $@", "--"] + list(args),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        stdin=subprocess.PIPE,
        env=os.environ.copy()
    )
