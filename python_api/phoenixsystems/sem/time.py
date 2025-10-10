#
# Time management for SEM simulators
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

import datetime
import math
import time


class TimeMachine:
    start_time: datetime.datetime
    speedup: int

    last_update_utc: int
    last_update_host_mono: int
    running: bool

    @staticmethod
    def _get_host_mono() -> int:
        return int(math.floor(time.monotonic()))

    def __init__(
        self,
        start_time: datetime.datetime,
        speedup: int,
        initialize_stopped: bool = False,
    ):
        self.start_time = start_time
        self.last_update_utc = int(math.floor(start_time.timestamp()))
        self.speedup = speedup
        self.running = False
        if not initialize_stopped:
            self.resume()

    def resume(self):
        if not self.running:
            self.last_update_host_mono = TimeMachine._get_host_mono()
            self.running = True

    def stop(self):
        if self.running:
            self.running = False
            self.last_update_utc = self.get_time_utc()

    def set_speedup(self, speedup: int):
        if self.running:
            self.stop()
            self.speedup = speedup
            self.resume()
        else:
            self.speedup = speedup

    def set_time_utc(self, now: datetime.datetime):
        self.last_update_utc = int(math.floor(now))
        self.last_update_host_mono = TimeMachine._get_host_mono()


    def get_time_utc(self) -> int:
        if self.running:
            return self.last_update_utc + (TimeMachine._get_host_mono() - self.last_update_host_mono) * self.speedup
        else:
            return self.last_update_utc
