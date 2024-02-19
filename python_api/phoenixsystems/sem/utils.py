#
# Utils for the Python wrapper
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

from dataclasses import dataclass
from ctypes import (
    c_double,
    Structure,
)


class MetersimException(Exception):
    pass


@dataclass
class DataInstant:
    u: list[float]
    i: list[float]
    i_n: float
    ui_angle: list[float]
    pp_angle: list[float]


@dataclass
class DataEnergy:
    active_plus: int
    active_minus: int
    reactive: list[int]
    apparent_plus: int
    apparent_minus: int

    def __str__(self) -> str:
        return f"""
    +A  [Ws]:     {self.active_plus}
    -A  [Ws]:     {self.active_minus}
    +Ri [vars]:   {self.reactive[0]}
    +Rc [vars]:   {self.reactive[1]}
    -Ri [vars]:   {self.reactive[2]}
    -Rc [vars]:   {self.reactive[3]}
    +S  [VAs]:    {self.apparent_plus}
    -S  [VAs]:    {self.apparent_minus}
    """


@dataclass
class DataPower:
    p: list[float]
    q: list[float]
    s: list[float]
    phi: list[float]


@dataclass
class DataVector:
    s: list[complex]
    u: list[complex]
    i: list[complex]
    i_n: complex


@dataclass
class DataThd:
    thdu: list[float]
    thdi: list[float]


class c_ComplexPy(Structure):
    _fields_ = [("real", c_double), ("imag", c_double)]

    def get_py_complex(self) -> complex:
        return complex(float(self.real), float(self.imag))

    @classmethod
    def get_c_struct(cls, x: complex):  # -> Self
        ret = c_ComplexPy()
        ret.real = x.real
        ret.imag = x.imag
        return ret


class c_dataVectorPy(Structure):
    _fields_ = [("s", c_ComplexPy * 3), ("u", c_ComplexPy * 3), ("i", c_ComplexPy * 3), ("iN", c_ComplexPy)]

    def get_py_struct(self) -> DataVector:
        return DataVector(
            [x.get_py_complex() for x in self.s],
            [x.get_py_complex() for x in self.u],
            [x.get_py_complex() for x in self.i],
            self.iN.get_py_complex(),
        )
