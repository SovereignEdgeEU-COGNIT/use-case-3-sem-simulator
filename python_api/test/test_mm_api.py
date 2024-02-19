#
# Unit tests for mm_api in Python
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

import time
import pytest
import phoenixsystems.sem.mm_api as mm_api
from phoenixsystems.sem.utils import DataInstant, DataEnergy, DataPower, DataThd
import math
import cmath
from pathlib import Path
from utils import sin, cos, compare_energy, compare_instant, compare_power


SCENARIO_DIR = Path("test/input/sc00")

# Configuration of the simulator (as defined in config.toml in SCENARIO_DIR)
initial_sem_config = {
    "serialNumber": "ABCD1234",
    "tariffCount": 5,
    "phaseCount": 3,
}

# Initial state of energy registers (as defined in config.toml in SCENARIO_DIR)
initial_energy_config = [
    DataEnergy(  # tariff 1, phase 1
        active_plus=0,
        active_minus=11,
        reactive=[0, 0, 0, 0],
        apparent_plus=0,
        apparent_minus=11,
    ),
    DataEnergy(  # tariff 2, phase 1
        active_plus=0,
        active_minus=22,
        reactive=[0, 0, 0, 0],
        apparent_plus=0,
        apparent_minus=22,
    ),
    DataEnergy(  # tariff 3, phase 1
        active_plus=0,
        active_minus=33,
        reactive=[0, 23, 21, 0],
        apparent_plus=0,
        apparent_minus=55,
    ),
]


@pytest.fixture()
def sem():
    sem_sim = mm_api.MMApiCtx(SCENARIO_DIR)
    return sem_sim


def test_config(sem):
    assert sem.get_serial_number() == initial_sem_config["serialNumber"]
    assert sem.get_tariff_count() == initial_sem_config["tariffCount"]
    assert sem.get_phase_count() == initial_sem_config["phaseCount"]


def test_instant(sem):
    # Instant values at timestamp 0 (as defined in updates.csv)
    expected = DataInstant(
        u=[210, 220, 230],
        i=[10, 20, 30],
        i_n=abs(10 + cmath.rect(20, cmath.pi * 225.0 / 180.0) + cmath.rect(30, cmath.pi * 275.0 / 180.0)),
        pp_angle=[120, 120],
        ui_angle=[0, 105, 35],
    )

    data_instant = sem.get_instant()
    compare_instant(data_instant, expected)


def test_power(sem):
    # Power at timestamp 0 (calculated according to instant values in updates.csv)
    expected = DataPower(
        p=[2100, 4400 * cos(105), 6900 * cos(35)],
        q=[0, 4400 * sin(105), 6900 * sin(35)],
        s=[2100, 4400, 6900],
        phi=[0, 105, 35],
    )

    data_power = sem.get_power()
    assert data_power is not None
    compare_power(data_power, expected)


def test_thd(sem):
    # Thd at timestamp 0 (as defined in updates.csv)
    expected = DataThd(
        thdu=[0.5, 0.512, 0.589],
        thdi=[0.689, 0.45, 0.25],
    )

    thdu = sem.get_thdu()
    thdi = sem.get_thdi()
    assert thdu == pytest.approx(expected.thdu)
    assert thdi == pytest.approx(expected.thdi)


# Test initial state of energy registers
def test_energy_init(sem):
    data_energy = [
        sem.get_energy_tariff(1)[0],  # tariff 1, phase 1
        sem.get_energy_tariff(2)[0],  # tariff 2, phase 1
        sem.get_energy_tariff(3)[0],  # tariff 3, phase 1
    ]

    for i in range(3):
        compare_energy(data_energy[i], initial_energy_config[i])


def test_energy(sem):
    # Test energy at tariff 0, when timestamp is in [10, 60], as then currentTariff is 4

    expected = [
        DataEnergy(
            active_plus=21000,
            active_minus=0,
            reactive=[0, 0, 0, 0],
            apparent_plus=21000,
            apparent_minus=0,
        ),
        DataEnergy(
            active_plus=0,
            active_minus=math.floor(-44000 * cos(105)),
            reactive=[0, math.floor(44000 * sin(105)), 0, 0],
            apparent_plus=0,
            apparent_minus=44000,
        ),
    ]

    time.sleep(0.2)
    data_energy = sem.get_energy_tariff(0)
    assert data_energy is not None

    for i in range(2):
        compare_energy(data_energy[i], expected[i])
