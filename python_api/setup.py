#
# Builder for SEM python package
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak, Jan Wiśniewski
#

# mypy: enable-error-code="explicit-override"
# pyright: reportImplicitOverride=true

import shutil
from setuptools import setup
import setuptools
import setuptools.command.build_py
import setuptools.command.develop
import setuptools.command.build
from distutils import log
from pathlib import Path
import typing
import subprocess

from typing_extensions import override

REPO_DIR = Path(__file__).resolve().parent.parent


class CustomBuild(setuptools.command.build.build):
    sub_commands = [("build_schema", None), *setuptools.command.build.build.sub_commands]  # type: ignore


class GenerateSubCommand(setuptools.Command, setuptools.command.build.SubCommand):
    """Boilerplate code for build-time package code/data generation"""

    editable_mode: bool
    build_lib: typing.Optional[str]

    _GEN_SOURCES: typing.ClassVar[list[str]]
    _GEN_RESULTS: typing.ClassVar[list[str]]

    @override
    def initialize_options(self):
        self.editable_mode = False
        self.build_base = None
        self.build_temp = None
        self.build_lib = None

    @override
    def finalize_options(self):
        self.set_undefined_options("build", ("build_base", "build_base"), ("build_lib", "build_lib"))
        self.set_undefined_options("build_ext", ("build_lib", "build_lib"), ("build_temp", "build_temp"))

    @override
    def get_source_files(self) -> list[str]:
        return [*self._GEN_SOURCES]

    @override
    def get_outputs(self) -> list[str]:
        return [f"{{build_lib}}/{dst}" for dst in self._GEN_RESULTS]

    @override
    def get_output_mapping(self) -> dict[str, str]:
        if self.editable_mode:
            return {"{{build_lib}}/{dst}": dst for dst in self._GEN_RESULTS}
        else:
            return {}

    def _generate(self, sdir: Path, bdir: Path):
        raise NotImplementedError

    @override
    def run(self):
        assert self.build_temp is not None

        # create directories for temporary result files
        for dst in self._GEN_RESULTS:
            tmp_dst = Path(self.build_temp) / dst
            tmp_dst.parent.mkdir(exist_ok=True, parents=True)
        sdir = Path(".")
        bdir = Path(self.build_temp)
        self._generate(sdir, bdir)

        # install generated files
        for dst in self._GEN_RESULTS:
            tmp_dst = Path(self.build_temp) / dst
            if self.editable_mode:
                lib_dst = Path(dst)
            else:
                assert self.build_lib is not None
                lib_dst = Path(self.build_lib) / dst
                lib_dst.parent.mkdir(exist_ok=True, parents=True)

            log.info(f"installing {tmp_dst} -> {lib_dst}")
            shutil.copyfile(tmp_dst, lib_dst)


_CLIB = "phoenixsystems/sem/libsemsim_py.so"


class BuildSchema(GenerateSubCommand):
    _GEN_SOURCES = []
    _GEN_RESULTS = [_CLIB]

    @override
    def _generate(self, sdir: Path, bdir: Path):
        log.info("compiling C library")
        subprocess.check_call(["git", "submodule", "init"], cwd=REPO_DIR)
        subprocess.check_call(["git", "submodule", "update"], cwd=REPO_DIR)
        subprocess.check_call(["cmake", "-B", "build", "-S", ".", "-DCMAKE_BUILD_TYPE=Release"], cwd=REPO_DIR)
        subprocess.check_call(["make", "clean"], cwd=REPO_DIR / "build")
        subprocess.check_call(["make"], cwd=REPO_DIR / "build")
        shutil.copyfile(REPO_DIR / "build" / "libsemsim_py.so", bdir / _CLIB)


setup(
    cmdclass={"build": CustomBuild, "build_schema": BuildSchema},
)
