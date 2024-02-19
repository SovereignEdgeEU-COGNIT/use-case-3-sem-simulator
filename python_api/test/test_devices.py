#
# Unit tests for devices in Python
#
# Copyright 2024 Phoenix Systems
# Author: Mateusz Kobak
#

import math
import cmath
from pathlib import Path
import pytest

import phoenixsystems.sem.metersim as metersim
from phoenixsystems.sem.device import (
    Device,
    InfoForDevice,
    DeviceResponse,
    METERSIM_NO_UPDATE_SCHEDULED,
)

SCENARIO_DIR = Path("test/input/sc00")


def with_arg(angle: float):
    return cmath.exp(complex(0, angle * math.pi / 180.0))


@pytest.fixture()
def sim():
    sem_sim = metersim.Metersim(SCENARIO_DIR)
    yield sem_sim
    sem_sim.destroy()


# Device with constant current
class MyDev1(Device):
    def update(self, info: InfoForDevice) -> DeviceResponse:
        res = DeviceResponse()
        res.next_update_time = METERSIM_NO_UPDATE_SCHEDULED
        res.current[0] = 1
        res.current[1] = 2 * cmath.exp(complex(0, 225.0 * math.pi / 180.0))
        res.current[2] = 3 * cmath.exp(complex(0, 275.0 * math.pi / 180.0))
        return res


# Device with current changing over time
class MyDev2(Device):
    def update(self, info: InfoForDevice) -> DeviceResponse:
        res = DeviceResponse()
        if info.now < 60:
            res.next_update_time = info.now + 1
            res.current[0] = info.now
            res.current[1] = info.now * cmath.exp(complex(0, 225.0 * math.pi / 180.0))
            res.current[2] = info.now * cmath.exp(complex(0, 275.0 * math.pi / 180.0))
        else:
            res.next_update_time = METERSIM_NO_UPDATE_SCHEDULED
        return res


# Device with dynamic switching
class MyDev3(Device):
    switch_on: bool = False

    def change_state(self, state: bool):
        old_state = self.switch_on
        self.switch_on = state
        if self.switch_on != old_state:
            self.notify()

    def update(self, info: InfoForDevice) -> DeviceResponse:
        res = DeviceResponse()
        if self.switch_on == 1:
            res.current[0] = 10
        else:
            res.current[0] = -15
        return res


# Device depending on voltage
class MyDev4(Device):
    def update(self, info: InfoForDevice) -> DeviceResponse:
        res = DeviceResponse()
        if info.voltage[0] == pytest.approx(240):
            res.current[0] = -30 * with_arg(15)
        if info.voltage[1] == pytest.approx(220 * with_arg(120)):
            res.current[1] = 15 * with_arg(225)
        if info.voltage[2] == pytest.approx(290 * with_arg(240)):
            res.current[2] = 7 * with_arg(275)

        return res


def test_constant_current(sim):
    sim.create_runner(False)

    dev = MyDev1()
    sim.add_device(dev)

    sim.resume()
    instant = sim.get_instant()
    assert instant.i[0] == pytest.approx(11)
    assert instant.i[1] == pytest.approx(22)
    assert instant.i[2] == pytest.approx(33)


def test_changing_current(sim):
    dev = MyDev2()
    sim.add_device(dev)

    sim.step_forward(59)

    instant = sim.get_instant()
    assert instant.i[0] == pytest.approx(69)
    assert instant.i[1] == pytest.approx(79)
    assert instant.i[2] == pytest.approx(89)

    sim.step_forward(1)

    uptime = sim.get_uptime()
    assert uptime == 60

    instant = sim.get_instant()
    assert instant.i[0] == pytest.approx(10)
    assert instant.i[1] == pytest.approx(20)
    assert instant.i[2] == pytest.approx(30)

    sim.step_forward(1000)

    instant = sim.get_instant()
    assert instant.i[0] == pytest.approx(10)
    assert instant.i[1] == pytest.approx(20)
    assert instant.i[2] == pytest.approx(30)


def test_dynamic_switching(sim):
    dev = MyDev3()
    dev.switch_on = True
    sim.add_device(dev)

    sim.step_forward(15)

    instant = sim.get_instant()
    assert instant.i[0] == pytest.approx(20)

    sim.step_forward(15)
    dev.change_state(False)
    sim.step_forward(15)

    instant = sim.get_instant()
    assert instant.i[0] == pytest.approx(5)
    vector = sim.get_vector()
    assert vector.i[0] == pytest.approx(-5)

    dev.change_state(True)
    sim.step_forward(10)

    instant = sim.get_instant()
    assert instant.i[0] == pytest.approx(20)
    vector = sim.get_vector()
    assert vector.i[0] == pytest.approx(20)


def test_voltageDependent(sim):
    dev = MyDev4()
    sim.add_device(dev)

    sim.step_forward(20)

    # The values of current are the sum (as complex numbers) of current taken from updates.csv and from the device
    vector = sim.get_vector()
    assert vector.i == pytest.approx([10 * with_arg(0), 35 * with_arg(225), 30 * with_arg(275)])

    sim.step_forward(60)

    vector = sim.get_vector()
    assert vector.i == pytest.approx([-20 * with_arg(15), 20 * with_arg(145), 30 * with_arg(275)])

    sim.step_forward(60)

    vector = sim.get_vector()
    assert vector.i == pytest.approx([10 * with_arg(95), 20 * with_arg(145), 37 * with_arg(275)])
