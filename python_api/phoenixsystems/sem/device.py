#
# Python wrapper for devices in SEM simulator
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

from abc import ABC, abstractmethod
from ctypes import (
    CDLL,
    byref,
    c_void_p,
    c_int32,
    c_int64,
    Structure,
    POINTER,
)
from dataclasses import dataclass
import threading
from typing import Any

from phoenixsystems.sem.utils import c_ComplexPy, MetersimException

METERSIM_NO_UPDATE_SCHEDULED = 2**31 - 1  # INT32_MAX


class c_metersim_deviceResponsePy_t(Structure):
    _fields_ = [
        ("current", c_ComplexPy * 3),
        ("nextUpdateTime", c_int32),
    ]


class c_metersim_infoForDevicePy_t(Structure):
    _fields_ = [
        ("voltage", c_ComplexPy * 3),
        ("now", c_int32),
        ("nowUtc", c_int64),
    ]


class DeviceResponse:
    current: list[complex]
    next_update_time: int

    def __init__(self, current: list[complex] | None = None, next_update_time: int = METERSIM_NO_UPDATE_SCHEDULED):
        if current is None:
            self.current = [0, 0, 0]
        else:
            self.current = current
        self.next_update_time = next_update_time

    def accumulate(self, other) -> None:
        for i in range(3):
            self.current[i] += other.current[i]

        self.next_update_time = min(self.next_update_time, other.next_update_time)

    def get_c_struct(self) -> c_metersim_deviceResponsePy_t:
        res = c_metersim_deviceResponsePy_t()
        for i in range(3):
            res.current[i] = c_ComplexPy.get_c_struct(self.current[i])
        res.nextUpdateTime = self.next_update_time
        return res


@dataclass
class InfoForDevice:
    voltage: list[complex]
    now: int
    nowUtc: int

    @staticmethod
    def get_from_c_struct(c_info: c_metersim_infoForDevicePy_t):  # -> Self
        info = InfoForDevice([0, 0, 0], -1, -1)
        for i in range(3):
            info.voltage[i] = complex(float(c_info.voltage[i].real), float(c_info.voltage[i].imag))
        info.now = c_info.now
        info.nowUtc = c_info.nowUtc
        return info


class Device(ABC):
    mgr: Any

    def notify(self) -> None:
        """Notifies the Simulator that the state has changed and it should poll the device for updates"""
        self.mgr.notify()

    def get_time(self) -> int:
        """Get current timestamp from the Simulator"""
        return self.mgr.get_time()

    @abstractmethod
    def update(self, info: InfoForDevice) -> DeviceResponse:
        """The device callback"""
        pass


class DeviceMgr:
    lib: CDLL
    devices: list[Device]
    ctx: c_void_p
    running: bool
    mgr_thread: threading.Thread
    metersim_ctx: c_void_p

    def __init__(self, lib: CDLL, metersimCtx: c_void_p) -> None:
        lib.devicePy_waitForWakeup.argtypes = [c_void_p, POINTER(c_metersim_infoForDevicePy_t)]
        lib.devicePy_waitForWakeup.restype = None

        lib.devicePy_setResponse.argtypes = [c_void_p, POINTER(c_metersim_deviceResponsePy_t)]
        lib.devicePy_setResponse.restype = None

        lib.devicePy_notify.argtypes = [c_void_p]
        lib.devicePy_notify.restype = None

        lib.devicePy_init.argtypes = [c_void_p]
        lib.devicePy_init.restype = c_void_p

        lib.devicePy_finish.argtypes = [c_void_p]
        lib.devicePy_finish.restype = None

        lib.devicePy_destroy.argtypes = [c_void_p]
        lib.devicePy_destroy.restype = None

        self.lib = lib
        self.devices = []
        self.metersim_ctx = metersimCtx

        self.ctx = lib.devicePy_init(metersimCtx)
        if self.ctx is None:
            raise MetersimException("Cannot initialize deviceMgr for Python")

        self.running = False
        self.mgr_thread = threading.Thread(target=self.run, args=[])
        self.mgr_thread.start()

    def destroy(self) -> None:
        self.running = False
        self.lib.devicePy_finish(self.ctx)
        self.mgr_thread.join()
        self.lib.devicePy_destroy(self.ctx)

    def add_device(self, dev: Device) -> None:
        dev.mgr = self
        self.devices.append(dev)

    def notify(self) -> None:
        self.lib.devicePy_notify(self.ctx)

    def get_time(self) -> int:
        now = c_int32()
        self.lib.metersim_getUptime(self.metersim_ctx, byref(now))
        return now.value

    def update(self, info: InfoForDevice) -> None:
        res = DeviceResponse()
        for dev in self.devices:
            resTmp = dev.update(info)
            res.accumulate(resTmp)

        self.lib.devicePy_setResponse(self.ctx, byref(res.get_c_struct()))

    def wait_on_cond(self) -> InfoForDevice:
        info = c_metersim_infoForDevicePy_t()
        self.lib.devicePy_waitForWakeup(self.ctx, byref(info))
        return InfoForDevice.get_from_c_struct(info)

    def run(self) -> None:
        self.running = True
        while self.running:
            info = self.wait_on_cond()
            self.update(info)
