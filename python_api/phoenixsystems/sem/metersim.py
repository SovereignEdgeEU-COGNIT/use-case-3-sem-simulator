#
# Python wrapper for SEM simulator API
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

from typing import Any, Callable
from pathlib import Path
import logging
from ctypes import (
    CDLL,
    CFUNCTYPE,
    c_int,
    c_float,
    c_double,
    c_char_p,
    c_uint64,
    c_ulong,
    c_void_p,
    c_int64,
    c_size_t,
    c_uint,
    c_int32,
    Structure,
    POINTER,
    create_string_buffer,
    sizeof,
    cast,
)

from phoenixsystems.sem.utils import (
    DataEnergy,
    DataInstant,
    DataPower,
    DataVector,
    DataThd,
    c_dataVectorPy,
    MetersimException,
)
from phoenixsystems.sem.device import (
    Device,
    DeviceMgr,
)
from phoenixsystems.sem.time import TimeMachine


"""

C Structures

"""


class c_metersim_instant_t(Structure):
    _fields_ = [
        ("frequency", c_float),
        ("voltage", c_double * 3),
        ("current", c_double * 3),
        ("currentNeutral", c_double),
        ("uiAngle", c_double * 3),
        ("ppAngle", c_double * 2),
    ]

    def get_py_struct(self) -> DataInstant:
        return DataInstant(
            [float(x) for x in self.voltage],
            [float(x) for x in self.current],
            float(self.currentNeutral),
            [float(x) for x in self.uiAngle],
            [float(x) for x in self.ppAngle],
        )


class c_metersim_eregister_t(Structure):
    _fields_ = [
        ("value", c_int64),
        ("fraction", c_double),
    ]


class c_metersim_energy_t(Structure):
    _fields_ = [
        ("activePlus", c_metersim_eregister_t),
        ("activeMinus", c_metersim_eregister_t),
        ("reactive", c_metersim_eregister_t * 4),
        ("apparentPlus", c_metersim_eregister_t),
        ("apparentMinus", c_metersim_eregister_t),
    ]

    def get_py_struct(self) -> DataEnergy:
        return DataEnergy(
            int(self.activePlus.value),
            int(self.activeMinus.value),
            [int(self.reactive[i].value) for i in range(4)],
            int(self.apparentPlus.value),
            int(self.apparentMinus.value),
        )


class c_metersim_power_t(Structure):
    _fields_ = [
        ("truePower", c_double * 3),
        ("reactivePower", c_double * 3),
        ("apparentPower", c_double * 3),
        ("phi", c_double * 3),
    ]

    def get_py_struct(self) -> DataPower:
        return DataPower(
            [float(x) for x in self.truePower],
            [float(x) for x in self.reactivePower],
            [float(x) for x in self.apparentPower],
            [float(x) for x in self.phi],
        )


class c_metersim_thd_t(Structure):
    _fields_ = [
        ("thdU", c_float * 3),
        ("thdI", c_float * 3),
    ]

    def get_py_struct(self) -> DataThd:
        return DataThd(
            [float(x) for x in self.thdU],
            [float(x) for x in self.thdI],
        )


LIB_DIR = Path(__file__).resolve().parent
lib = CDLL(str(LIB_DIR / "libsemsim_py.so"))

# Initialize function arg and ret types
lib.metersim_init.argtypes = [c_void_p]
lib.metersim_init.restype = c_void_p

lib.metersim_free.argtypes = [c_void_p]
lib.metersim_free.restype = None


lib.metersim_createRunner.argtypes = [c_void_p, c_int]
lib.metersim_createRunner.restype = c_int

lib.metersim_createRunnerWithCb.argtypes = [c_void_p, c_void_p, c_void_p]
lib.metersim_createRunnerWithCb.restype = c_int

lib.metersim_destroyRunner.argtypes = [c_void_p]
lib.metersim_destroyRunner.restype = None

lib.metersim_resume.argtypes = [c_void_p]
lib.metersim_resume.restype = c_int

lib.metersim_pause.argtypes = [c_void_p, c_int]
lib.metersim_pause.restype = c_int

lib.metersim_isRunning.argtypes = [c_void_p]
lib.metersim_isRunning.restype = c_int

lib.metersim_setSpeedup.argtypes = [c_void_p, c_int]
lib.metersim_setSpeedup.restype = c_int


lib.metersim_stepForward.argtypes = [c_void_p, c_int]
lib.metersim_stepForward.restype = c_int


lib.metersim_getTariffCount.argtypes = [c_void_p, POINTER(c_int)]
lib.metersim_getTariffCount.restype = None

lib.metersim_getTariffCurrent.argtypes = [c_void_p, POINTER(c_int)]
lib.metersim_getTariffCurrent.restype = None

lib.metersim_getSerialNumber.argtypes = [c_void_p, c_int, c_char_p, c_size_t]
lib.metersim_getSerialNumber.restype = c_int

lib.metersim_getTimeUTC.argtypes = [c_void_p, POINTER(c_int64)]
lib.metersim_getTimeUTC.restype = None

lib.metersim_setTimeUTC.argtypes = [c_void_p, c_int64]
lib.metersim_setTimeUTC.restype = None

lib.metersim_getUptime.argtypes = [c_void_p, POINTER(c_int32)]
lib.metersim_getUptime.restype = None

lib.metersim_getPhaseCount.argtypes = [c_void_p, POINTER(c_int)]
lib.metersim_getPhaseCount.restype = None

lib.metersim_getFrequency.argtypes = [c_void_p, POINTER(c_float)]
lib.metersim_getFrequency.restype = None

lib.metersim_getMeterConstant.argtypes = [c_void_p, POINTER(c_uint)]
lib.metersim_getMeterConstant.restype = None

lib.metersim_getInstant.argtypes = [c_void_p, POINTER(c_metersim_instant_t)]
lib.metersim_getInstant.restype = None

lib.metersim_getEnergyTotal.argtypes = [c_void_p, POINTER(c_metersim_energy_t)]
lib.metersim_getEnergyTotal.restype = None

lib.metersim_getEnergyTariff.argtypes = [c_void_p, POINTER(c_metersim_energy_t), c_int]
lib.metersim_getEnergyTariff.restype = c_int

lib.metersim_getPower.argtypes = [c_void_p, POINTER(c_metersim_power_t)]
lib.metersim_getPower.restype = None

lib.metersim_getVectorPy.argtypes = [c_void_p, POINTER(c_dataVectorPy)]
lib.metersim_getVectorPy.restype = None

lib.metersim_getThd.argtypes = [c_void_p, POINTER(c_metersim_thd_t)]
lib.metersim_getThd.restype = None


class Metersim:
    ctx: c_void_p
    dev_mgr: DeviceMgr
    destroyed: bool = False

    def __init__(self, scenario_dir: Path) -> None:
        self.ctx = lib.metersim_init(c_char_p(str(scenario_dir).encode()))
        if not self.ctx:
            raise MetersimException("Could not initialize metersim context")
        self.dev_mgr = DeviceMgr(lib, self.ctx)

    def __del__(self) -> None:
        if not self.destroyed:
            logging.warning("Destroy was never called on this Metersim")
            self.destroy()

    def _call_helper(
        self,
        api_foo: Callable[[c_void_p, Any], None],
        c_type: type,
        size: int = 1,
    ) -> Any:
        ret_tmp = create_string_buffer(size * sizeof(c_type))
        ret = cast(ret_tmp, POINTER(c_type))
        api_foo(self.ctx, ret)
        return ret

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.destroy()

    def destroy(self):
        if not self.destroyed:
            self.destroyed = True
            self.destroy_runner()
            self.dev_mgr.destroy()
            lib.metersim_free(self.ctx)

    def create_runner(self, start: bool = False) -> None:
        status = lib.metersim_createRunner(self.ctx, c_int(int(start)))
        if status != 0:
            raise MetersimException("Could not initialize runner")

    def create_runner_custom_time(self, time_machine: TimeMachine) -> None:
        @CFUNCTYPE(c_ulong, c_void_p)
        def c_time_cb(ptr: c_void_p) -> c_ulong:
            now = time_machine.get_time_utc()
            ret = now
            return ret
        self.c_time_cb = c_time_cb
        status = lib.metersim_createRunnerWithCb(self.ctx, self.c_time_cb, None)
        if status != 0:
            raise MetersimException("Could not initialize runner")

    def destroy_runner(self) -> None:
        lib.metersim_destroyRunner(self.ctx)

    def resume(self) -> None:
        status = lib.metersim_resume(self.ctx)
        if status != 0:
            raise MetersimException("Resume not possible. Runner not initialized.")

    def pause(self, when: int) -> None:
        status = lib.metersim_pause(self.ctx, c_int(when))
        if status != 0:
            raise MetersimException("Pause not possible. Runner not initialized.")

    def is_running(self) -> bool:
        ret = lib.metersim_isRunning(self.ctx)
        return ret == 1

    def set_speedup(self, speedup: int) -> None:
        status = lib.metersim_setSpeedup(self.ctx, c_int(speedup))
        if status != 0:
            raise MetersimException("Setting speedup not possible.")

    def add_device(self, device: Device) -> None:
        self.dev_mgr.add_device(device)

    def notify_device_mgr(self) -> None:
        self.dev_mgr.notify()

    def step_forward(self, seconds: int) -> None:
        status = lib.metersim_stepForward(self.ctx, c_int(seconds))
        if status != 0:
            raise MetersimException("Step-forward not allowed. Runner still running.")

    def get_tariff_count(self) -> int:
        ret = self._call_helper(lib.metersim_getTariffCount, c_int)
        return ret.contents.value

    def get_tariff_current(self) -> int:
        ret = self._call_helper(lib.metersim_getTariffCurrent, c_int)
        return ret.contents.value

    def get_serial_number(self) -> str:
        ret_tmp = create_string_buffer(32)
        status = lib.metersim_getSerialNumber(self.ctx, 0, ret_tmp, c_size_t(32))
        if status < 0:
            raise MetersimException("")
        ret = cast(ret_tmp, c_char_p)
        return ret.value.decode()

    def get_time_utc(self) -> int:
        ret = self._call_helper(lib.metersim_getTimeUTC, c_int64)
        return ret.contents.value

    def set_time_utc(self, time) -> None:
        lib.metersim_setTimeUTC(self.ctx, c_int64(time))

    def get_uptime(self) -> int:
        ret = self._call_helper(lib.metersim_getUptime, c_int)
        return ret.contents.value

    def get_phase_count(self) -> int:
        ret = self._call_helper(lib.metersim_getPhaseCount, c_int)
        return ret.contents.value

    def get_frequency(self) -> int:
        ret = self._call_helper(lib.metersim_getFrequency, c_float)
        return ret.contents.value

    def get_meter_constant(self) -> int:
        ret = self._call_helper(lib.metersim_getMeterConstant, c_uint)
        return ret.contents.value

    def get_instant(self) -> DataInstant:
        ret = self._call_helper(lib.metersim_getInstant, c_metersim_instant_t)
        return ret.contents.get_py_struct()

    def get_energy_total(self) -> DataEnergy:
        ret = self._call_helper(lib.metersim_getEnergyTotal, c_metersim_energy_t)
        return ret.contents.get_py_struct()

    def get_energy_tariff(self, tariff: int) -> list[DataEnergy]:
        ret = self._call_helper(
            (lambda ctx, data: lib.metersim_getEnergyTariff(ctx, data, tariff)),
            c_metersim_energy_t,
            3,
        )
        return [ret[i].get_py_struct() for i in range(3)]

    def get_power(self) -> DataPower:
        ret = self._call_helper(lib.metersim_getPower, c_metersim_power_t)
        return ret.contents.get_py_struct()

    def get_vector(self) -> DataVector:
        ret = self._call_helper(lib.metersim_getVectorPy, c_dataVectorPy)
        return ret.contents.get_py_struct()

    def get_thd(self) -> DataThd:
        ret = self._call_helper(lib.metersim_getThd, c_metersim_thd_t)
        return ret.contents.get_py_struct()
