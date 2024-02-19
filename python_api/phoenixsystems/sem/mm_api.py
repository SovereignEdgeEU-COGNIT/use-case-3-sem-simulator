#
# Python wrapper for meter message API
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

import logging
from typing import Any, Callable, List
from pathlib import Path
from ctypes import (
    CDLL,
    c_int,
    c_float,
    c_char_p,
    c_void_p,
    c_int64,
    c_size_t,
    c_uint,
    c_int32,
    c_char,
    Structure,
    POINTER,
    create_string_buffer,
    sizeof,
    cast,
    byref,
)

from phoenixsystems.sem.utils import DataEnergy, DataInstant, DataPower, DataVector, c_dataVectorPy


"""

C Structures

"""


class c_mm_address(Structure):
    _fields_ = [
        ("typ", c_int),
        ("addr", c_char_p),
    ]


class c_mme_dataInstant(Structure):
    _fields_ = [
        ("u", c_float * 3),
        ("i", c_float * 3),
        ("in", c_float),
        ("uiAngle", c_float * 3),
        ("ppAngle", c_float * 2),
    ]

    def get_py_struct(self):
        return DataInstant(
            [float(x) for x in self.u],
            [float(x) for x in self.i],
            float(getattr(self, "in")),
            [float(x) for x in self.uiAngle],
            [float(x) for x in self.ppAngle],
        )


class c_mme_dataEnergy(Structure):
    _fields_ = [
        ("activePlus", c_int64),
        ("activeMinus", c_int64),
        ("reactive", c_int64 * 4),
        ("apparentPlus", c_int64),
        ("apparentMinus", c_int64),
    ]

    def get_py_struct(self):
        return DataEnergy(
            int(self.activePlus),
            int(self.activeMinus),
            [int(x) for x in self.reactive],
            int(self.apparentPlus),
            int(self.apparentMinus),
        )


class c_mme_dataPower(Structure):
    _fields_ = [
        ("p", c_float * 3),
        ("q", c_float * 3),
        ("s", c_float * 3),
        ("phi", c_float * 3),
    ]

    def get_py_struct(self):
        return DataPower(
            [float(x) for x in self.p],
            [float(x) for x in self.q],
            [float(x) for x in self.s],
            [float(x) for x in self.phi],
        )


LIB_DIR = Path(__file__).resolve().parent
lib = CDLL(str(LIB_DIR / "libsemsim_py.so"))

# Initialize function arg and ret types
lib.mm_init.argtypes = []
lib.mm_init.restype = c_void_p

lib.mm_free.argtypes = [c_void_p]
lib.mm_free.restype = c_int

lib.mm_connect.argtypes = [c_void_p, POINTER(c_mm_address), c_void_p]
lib.mm_connect.restype = c_int

lib.mm_disconnect.argtypes = [c_void_p]
lib.mm_disconnect.restype = c_int

lib.mm_getTariffCount.argtypes = [c_void_p, POINTER(c_int)]
lib.mm_getTariffCount.restype = c_int

lib.mm_getTariffCurrent.argtypes = [c_void_p, POINTER(c_int)]
lib.mm_getTariffCurrent.restype = c_int

lib.mm_getSerialNumber.argtypes = [c_void_p, c_int, c_char_p, c_size_t]
lib.mm_getSerialNumber.restype = c_int

lib.mm_getTimeUTC.argtypes = [c_void_p, POINTER(c_int64)]
lib.mm_getTimeUTC.restype = c_int

lib.mm_getUptime.argtypes = [c_void_p, POINTER(c_int32)]
lib.mm_getUptime.restype = c_int

lib.mm_getTemperature.argtypes = [c_void_p, POINTER(c_float)]
lib.mm_getTemperature.restype = c_int

lib.mm_getMeterDomain.argtypes = [c_void_p, POINTER(c_uint)]
lib.mm_getMeterDomain.restype = c_int

lib.mme_getPhaseCount.argtypes = [c_void_p, POINTER(c_int)]
lib.mme_getPhaseCount.restype = c_int

lib.mme_getFrequency.argtypes = [c_void_p, POINTER(c_float)]
lib.mme_getFrequency.restype = c_int

lib.mme_getMeterConstant.argtypes = [c_void_p, POINTER(c_uint)]
lib.mme_getMeterConstant.restype = c_int

lib.mme_getInstant.argtypes = [c_void_p, POINTER(c_mme_dataInstant)]
lib.mme_getInstant.restype = c_int

lib.mme_getEnergyTotal.argtypes = [c_void_p, POINTER(c_mme_dataEnergy)]
lib.mme_getEnergyTotal.restype = c_int

lib.mme_getEnergyTariff.argtypes = [c_void_p, POINTER(c_mme_dataEnergy), c_int]
lib.mme_getEnergyTariff.restype = c_int

lib.mme_getPower.argtypes = [c_void_p, POINTER(c_mme_dataPower)]
lib.mme_getPower.restype = c_int

lib.mme_getVectorPy.argtypes = [c_void_p, POINTER(c_dataVectorPy)]
lib.mme_getVectorPy.restype = c_int

lib.mme_getThdU.argtypes = [c_void_p, POINTER(c_float)]
lib.mme_getThdU.restype = c_int

lib.mme_getThdI.argtypes = [c_void_p, POINTER(c_float)]
lib.mme_getThdI.restype = c_int


class MMApiException(Exception):
    status_code: int

    def __init__(self, msg: str = "", status: int = -1):
        super().__init__(msg)
        self.status_code = status


class MMApiCtx:
    ctx: c_void_p
    destroyed: bool

    def __init__(self, scenario_dir: Path) -> None:
        self.destroyed = False
        self.ctx = lib.mm_init()
        if not self.ctx:
            raise MMApiException("Could not initialize MM_API context")
        address = c_mm_address(1, c_char_p(str(scenario_dir).encode()))
        status = lib.mm_connect(self.ctx, byref(address), None)
        if status < 0:
            raise MMApiException(status=status)

    def _call_helper(
        self,
        api_foo: Callable[[c_void_p, Any], int],
        c_type: type,
        size: int = 1,
    ) -> Any:
        ret_tmp = create_string_buffer(size * sizeof(c_type))
        ret = cast(ret_tmp, POINTER(c_type))
        status = api_foo(self.ctx, ret)
        if status < 0:
            raise MMApiException(status=status)
        return ret

    """
    MM_API functions
    """

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        del self

    def __del__(self) -> None:
        if not self.destroyed:
            logging.warning("Destroy was never called on this MMApiCtx")
            self.destroy()

    def destroy(self) -> None:
        """Stops simulation"""
        if not self.destroyed:
            status = lib.mm_disconnect(self.ctx)
            if status != 0:
                raise MMApiException(status=status)
            status = lib.mm_free(self.ctx)
            if status != 0:
                raise MMApiException(status=status)

    def get_tariff_count(self) -> int:
        """Returns the number of available tariffs."""
        ret = self._call_helper(lib.mm_getTariffCount, c_int)
        return ret.contents.value

    def get_tariff_current(self) -> int:
        """Returns index of the current tariff."""
        ret = self._call_helper(lib.mm_getTariffCurrent, c_int)
        return ret.contents.value

    def get_serial_number(self) -> str:
        """Returns serial numbers of the meter"""
        buf_len = c_size_t(32)  # TODO: make some constant maybe?
        ret = self._call_helper(
            lambda ctx, ret: lib.mm_getSerialNumber(ctx, 0, ret, buf_len),
            c_char,
            buf_len.value,
        )
        ret = cast(ret, c_char_p)
        return ret.value.decode()

    def get_time_utc(self) -> int:
        """Returns current timestamp from the simulator"""
        ret = self._call_helper(lib.mm_getTimeUTC, c_int64)
        return ret.contents.value

    def get_uptime(self) -> int:
        """Returns uptime (seconds)"""
        ret = self._call_helper(lib.mm_getUptime, c_int)
        return ret.contents.value

    def get_temperature(self) -> float:
        """Returns ambient temperature (deg.C)"""
        ret = self._call_helper(lib.mm_getTemperature, c_float)
        return ret.contents.value

    def get_meter_domain(self) -> int:
        """Returns metering domain and capabilities."""
        ret = self._call_helper(lib.mm_getMeterDomain, c_uint)
        return ret.contents.value

    def get_phase_count(self) -> int:
        """Returns the number of phases."""
        ret = self._call_helper(lib.mme_getPhaseCount, c_int)
        return ret.contents.value

    def get_frequency(self) -> float:
        """Returns mains frequency (Hz)."""
        ret = self._call_helper(lib.mme_getFrequency, c_float)
        return ret.contents.value

    def get_meter_constant(self) -> int:
        """Returns meter constant (Ws)."""
        ret = self._call_helper(lib.mm_getMeterConstant, c_uint)
        return ret.contents.value

    def get_instant(self) -> DataInstant:
        """Returns instantaneous values of Urms, Irms, and UI and phase-phase angles"""
        data = self._call_helper(lib.mme_getInstant, c_mme_dataInstant)
        ret = data.contents.get_py_struct()
        return ret

    def get_energy_total(self) -> DataEnergy:
        """Returns energy registers grand total (all phases, all tariffs)."""
        data = self._call_helper(lib.mme_getEnergyTotal, c_mme_dataEnergy)
        ret = data.contents.get_py_struct()
        return ret

    def get_energy_tariff(self, tariff: int) -> List[DataEnergy]:
        """Returns energy registers particularly per tariff."""
        data = self._call_helper(
            lambda ctx, ret: lib.mme_getEnergyTariff(ctx, ret, tariff),
            c_mme_dataEnergy,
            3,
        )
        ret = [data[i].get_py_struct() for i in range(3)]
        return ret

    def get_power(self) -> DataPower:
        """Returns power triangle (P, Q, S, phi angle)."""
        data = self._call_helper(lib.mme_getPower, c_mme_dataPower)
        ret = data.contents.get_py_struct()
        return ret

    def get_vector(self) -> DataVector:
        """Returns vector data on complex number plane (fundamental freq)."""
        data = self._call_helper(lib.mme_getVectorPy, c_dataVectorPy)
        ret = data.contents.get_py_struct()
        return ret

    def get_thdi(self) -> List[float]:
        """Returns total harmonic distortion for current per phase."""
        data = self._call_helper(lib.mme_getThdI, c_float, 3)
        ret = [float(data[i]) for i in range(3)]
        return ret

    def get_thdu(self) -> List[float]:
        """Returns total harmonic distortion for voltage per phase."""
        data = self._call_helper(lib.mme_getThdU, c_float, 3)
        ret = [float(data[i]) for i in range(3)]
        return ret
