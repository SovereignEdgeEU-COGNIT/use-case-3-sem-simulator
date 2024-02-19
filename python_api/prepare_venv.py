#!/usr/bin/env python3
#
# Python virtual environment creator for SEM package
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#


import argparse
import sys
import shutil

from pathlib import Path
from subprocess import check_call


def print_cyan(msg: str):
    print("\033[36m {}\033[00m".format(msg))


REPO_DIR = Path(__file__).resolve().parent.parent
VENV_DIR = REPO_DIR / "_venv"
PY_DIR = REPO_DIR / "python_api"

parser = argparse.ArgumentParser()
parser.add_argument(
    "--force",
    action="store_true",
    help="remove old virtual environment if exists",
)

cmd_args = parser.parse_args()


def error(msg):
    print(msg, file=sys.stderr)
    sys.exit(1)


if VENV_DIR.exists():
    if not cmd_args.force:
        error(
            f"ERROR: venv directory already exists: {VENV_DIR.absolute()}\n"
            f"Add '--force' flag to remove it before create new one"
        )

    print_cyan(f"deleting old virtual environment: {VENV_DIR}")
    shutil.rmtree(VENV_DIR)

print_cyan(f"Creating new venv: {VENV_DIR}")
check_call(["python3", "-m", "venv", VENV_DIR], cwd=REPO_DIR)

pip_install = [VENV_DIR / "bin" / "python3", "-m", "pip", "install"]

print_cyan("Upgrading pip")
check_call([*pip_install, "--upgrade", "pip"])


print_cyan("Installing sem-simulator package")
check_call([*pip_install, "--editable", PY_DIR / ""])

print_cyan("Installing test requirements")
check_call([*pip_install, "-r", PY_DIR / "test" / "requirements.txt"])

print_cyan("virtual environment created")
print_cyan(f"to enter run: source {VENV_DIR}/bin/activate")
