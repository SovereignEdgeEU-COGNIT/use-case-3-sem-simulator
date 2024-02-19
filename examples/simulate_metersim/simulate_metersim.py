#!/usr/bin/env python3
#
# Basic simulation app
#
# Copyright 2023-2024 Phoenix Systems
# Author: Mateusz Kobak
#

import time
from pathlib import Path
import phoenixsystems.sem.metersim as metersim


SCENARIO_PATH = Path(__file__).resolve().parent / "scenario"


with metersim.Metersim(SCENARIO_PATH) as sim:

    sim.create_runner(False)
    sim.set_speedup(10)
    sim.resume()

    # Get the number of phases
    phases = sim.get_phase_count()
    sim.pause(when=100)

    # Print energy registers for current tariff for each phase
    for i in range(120):
        tariff = sim.get_tariff_current()
        uptime = sim.get_uptime()

        energy = sim.get_energy_total()

        print("Total energy at time: ", uptime)
        print("Current tariff is: ", tariff)
        print("")
        print(str(energy))
        time.sleep(0.1)

    sim.step_forward(100)
    tariff = sim.get_tariff_current()
    uptime = sim.get_uptime()
    assert uptime == 200

    energy = sim.get_energy_total()

    print("Total energy at time: ", uptime)
    print("Current tariff is: ", tariff)
    print("")
    print(str(energy))
