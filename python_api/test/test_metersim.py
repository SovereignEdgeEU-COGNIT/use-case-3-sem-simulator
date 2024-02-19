#
# Unit tests for metersim in Python
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#


import time
import pytest
import phoenixsystems.sem.metersim as metersim
from phoenixsystems.sem.utils import DataInstant, DataEnergy, DataPower, DataThd
import math
import cmath
from pathlib import Path
from utils import sin, cos, compare_energy, compare_instant, compare_power
from typing import List


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
    semSim = metersim.Metersim(SCENARIO_DIR)
    yield semSim
    semSim.destroy()


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
    assert data_instant is not None
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

    thd = sem.get_thd()
    assert thd.thdu == pytest.approx(expected.thdu)
    assert thd.thdi == pytest.approx(expected.thdi)


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
    # Energy at timestamp 10, calculated according to instant values in updates.csv
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

    sem.step_forward(10)
    data_energy = sem.get_energy_tariff(0)
    assert data_energy != None

    for i in range(2):
        compare_energy(data_energy[i], expected[i])


def check_uptime(actual: int, prev_uptime: int, speedup: int, real_time_elapsed: float, epsilon: float = 0.001):
    assert actual >= prev_uptime + speedup * real_time_elapsed
    assert actual <= prev_uptime + speedup * (real_time_elapsed + epsilon) + 1


def test_flow(sem):
    # Testing pauses and accuracy of the speedup

    sem.create_runner(False)

    # Test scheduled pausing
    speedup = 1000
    sem.set_speedup(speedup)
    sem.pause(when=50)
    sem.resume()
    time.sleep(0.1)
    uptime = sem.get_uptime()
    assert uptime == 50
    prev_uptime = uptime

    # Test immediate pausing
    sem.resume()
    time.sleep(0.1)
    sem.pause(when=0)
    uptime = sem.get_uptime()  # expected 150
    check_uptime(actual=uptime, prev_uptime=prev_uptime, speedup=speedup, real_time_elapsed=0.1)
    prev_uptime = uptime

    # Test immediate pausing
    sem.resume()
    time.sleep(0.1)
    sem.pause(when=0)
    uptime = sem.get_uptime()  # expected 250
    check_uptime(actual=uptime, prev_uptime=prev_uptime, speedup=speedup, real_time_elapsed=0.1)
    prev_uptime = uptime

    # Test uptime without pausing
    sem.resume()
    time.sleep(0.3)
    uptime = sem.get_uptime()  # expected 550
    check_uptime(actual=uptime, prev_uptime=prev_uptime, speedup=speedup, real_time_elapsed=0.3)
    prev_uptime = uptime

    # Test speedup decrease without pausing
    speedup = 100
    sem.set_speedup(speedup)
    time.sleep(0.1)
    sem.pause(when=0)
    uptime = sem.get_uptime()  # expected 560
    check_uptime(actual=uptime, prev_uptime=prev_uptime, speedup=speedup, real_time_elapsed=0.1)
    prev_uptime = uptime

    # Test speedup increase
    sem.pause(when=600)
    sem.set_speedup(1000)
    sem.resume()
    time.sleep(0.1)
    uptime = sem.get_uptime()
    assert uptime == 600

    # Test stepForward with runner present, but not running
    sem.step_forward(100)
    energy = sem.get_energy_tariff(0)
    assert energy[0].apparent_minus == pytest.approx(60 * 270 * 10 + 520 * 300 * 10)

    # Test raising exception when trying to step forward, while runner is running
    sem.pause(when=1500)
    sem.resume()
    with pytest.raises(metersim.MetersimException):
        sem.step_forward(100)

    # Test isRunning
    while sem.is_running():
        assert uptime < 1500
        time.sleep(0.05)
        uptime = sem.get_uptime()
    uptime = sem.get_uptime()
    assert uptime == 1500
