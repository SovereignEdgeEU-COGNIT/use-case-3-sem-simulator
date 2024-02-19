#!/usr/bin/env python3
#
# Basic simulation app
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

import time
from pathlib import Path
from phoenixsystems.sem.mm_api import MMApiCtx


SCENARIO_PATH = Path(__file__).resolve().parent / "scenario"


with MMApiCtx(SCENARIO_PATH) as sim:

    # Get the number of phases
    phases = sim.get_phase_count()

    # Print energy registers for current tariff for each phase
    for i in range(360):
        tariff = sim.get_tariff_current()
        energy = sim.get_energy_total()
        uptime = sim.get_uptime()

        print("Total energy at time: ", uptime)
        print("Current tariff is: ", tariff)
        print("")
        print(str(energy))
        time.sleep(0.1)
