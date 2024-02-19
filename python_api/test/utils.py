from phoenixsystems.sem.utils import DataEnergy, DataInstant, DataPower
import math
import pytest


def sin(angle: float) -> float:
    return math.sin(angle * math.pi / 180.0)


def cos(angle: float) -> float:
    return math.cos(angle * math.pi / 180.0)


# Energy register may have value 1 Ws less then expected, as internally casts from double to int64_t are used
def compare_energy_reg(actual: int, expected: int) -> bool:
    return expected == actual or expected - 1 == actual


def compare_energy(actual: DataEnergy, expected: DataEnergy) -> None:
    assert compare_energy_reg(actual.active_plus, expected.active_plus)
    assert compare_energy_reg(actual.active_minus, expected.active_minus)
    assert compare_energy_reg(actual.reactive[0], expected.reactive[0])
    assert compare_energy_reg(actual.reactive[1], expected.reactive[1])
    assert compare_energy_reg(actual.reactive[2], expected.reactive[2])
    assert compare_energy_reg(actual.reactive[3], expected.reactive[3])
    assert compare_energy_reg(actual.apparent_plus, expected.apparent_plus)
    assert compare_energy_reg(actual.apparent_minus, expected.apparent_minus)


def compare_instant(actual: DataInstant, expected: DataInstant) -> None:
    assert actual.u == pytest.approx(expected.u)
    assert actual.i == pytest.approx(expected.i)
    assert actual.pp_angle == pytest.approx(expected.pp_angle)
    assert actual.ui_angle == pytest.approx(expected.ui_angle)
    assert actual.i_n == pytest.approx(expected.i_n)


def compare_power(actual: DataPower, expected: DataPower) -> None:
    assert actual.phi == pytest.approx(expected.phi)
    assert actual.p == pytest.approx(expected.p)
    assert actual.q == pytest.approx(expected.q)
    assert actual.s == pytest.approx(expected.s)
