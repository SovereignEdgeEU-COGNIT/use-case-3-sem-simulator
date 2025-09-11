# SEM Simulator

Simulator of Smart Energy Meter.

This project is a simulation facility of the Smart Energy Meter (SEM) running predefined scenarios. It is mainly aimed to simplify the process of developing SEM user applications.

Two interfaces of the Simulator are provided;

* `metersim` (simulation API) – Gives the user more control over the flow of the simulation.
* `mm_api` (metermsg API) – This is the actual interface for the user application on SEM to get metrological data.

We provide APIs in C and in Python.

## Building
First prepare the submodules:
```
git submodule init
git submodule update
```

Use `cmake` to build the Simulator library, tests and examples. In `sem-simulator` directory call:
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cd build
make
```

The `CMAKE_BUILD_TYPE` can be set to `Release` or `Debug`. The only difference is that the latter provides more logs.

## Config files of the simulation scenario
The config files define the initial parameters of the scenario and provide a list of parameter updates, which change mutable parameters (like voltage and current) at given moments of the simulation.

#### Input directory
The input directory should contain two files:
* `config.toml` – the initial configuration of the simulation
* `updates.csv` – all the parameter updates

#### Structure of `config.toml`
The file `config.toml` sets the non-mutable parameters of the simulation scenario and the initial values of the energy registers.

In this example of `config.toml` all the possible parameters are set. There are no parameters that are obligatory to be set
```toml
serialNumber = "ABCD1234"
tariffCount = 4
phaseCount = 3
meterConstant = 7200
speedup = 4

[[tariff]] # Tariff 0

[[tariff]] # Tariff 1

[tariff.phase1] # Initial energy registers on tariff 1, phase 1
# All the registers are set here
activeMinus = 11 # (Ws)
activePlus = 234 # (Ws)
reactive1 = 15   # (vars)
reactive2 = 31   # (vars)
reactive3 = 8900 # (vars)
reactive4 = 9001 # (vars)

[[tariff]] # Tariff 2

[tariff.phase3] # Initial energy registers on tariff 2, phase 3
activeMinus = 77

[[tariff]] # Tariff 3

[[tariff]] # Tariff 4

[tariff.phase1] # Initial energy registers on tariff 4, phase 1
activeMinus = 33
reactive2 = 23
reactive3 = 21

```
Note: `speedup` described the speed-up factor that accelerates the time lapse. Here the time within the simulation will pass 4 times faster than the real time.

#### Structure of `updates.csv`
Lines of the file correspond to consecutive updates of the parameters. Below we show the content of the file `test/input/sc00/updates.csv` in a form of a table.

|timestamp|currentTariff|frequency|U0 |U1 |U2 |I0 |I1 |I2 |ui_angle0|ui_angle1|ui_angle2|thdU0|thdU1|thdU2|thdI0|thdI1|thdI2|
|---------|--------------|---------|---|---|---|---|---|---|---------|---------|---------|-----|-----|-----|-----|-----|-----|
|0        |0             |50       |210|220|230|10 |20 |30 |0        |25       |35       |2.1  |2.2  |2.3  |2.4  |2.5  |2.6  |
|60       |0             |50       |310|320|330|10 |20 |30 |15       |25       |35       |2.5  |2.5  |2.5  |2.5  |2.5  |2.5  |
|120      |0             |50       |410|420|430|10 |20 |30 |95       |25       |35       |2.5  |2.5  |2.5  |2.5  |2.5  |2.5  |
|180      |0             |50       |510|520|530|10 |20 |30 |110      |25       |35       |2.5  |2.5  |2.5  |2.5  |2.5  |2.5  |

Note that:
* Column `timestamp` describes the moment of the simulation (in seconds), when an update will be introduced. In the example above, the updates are scheduled for timestamps 0, 60, 120, and 180 of the simulation.
* The `timestamp` refers to the "simulated" time lapse. So if the speedup is set to 5, then the 2nd update will be introduced after 12 seconds from the start of the simulation.
* When a cell in `updates.csv` is left blank it does not change the value set by previous update.

## Usage of the `metersim` API
The `metersim` API allows an advanced control over the simulation. The user application should include the following files:

* `include/metersim/metersim.h` – API function definitions
* `include/metersim/metersim_types.h` – API type definition

The simulation can be performed in two different ways, which we describe below:

### Simulator without runner
Here the simulator does not mock the time lapse. It only calculates the state of the Smart Energy Meter after given steps.

```c
	metersim_instant_t instant;
	metersim_energy_t energy[3];

	/* Allocate memory for the simulator */
	mctx = metersim_init("input_dir"); /* Pass scenario directory */
	if (mctx == NULL) {
		printf("Unable to create metersim instance.\n");
		return EXIT_FAILURE;
	}

	/* Step forward by 100 seconds */
	metersim_stepForward(mctx, 100);

	/* Get instant parameters at timestamp 100 */
	metersim_getInstant(mctx, &instant);

	/* Step forward by 50 seconds */
	metersim_stepForward(mctx, 50);

	/* Get energy registers at tariff 2 and timestamp 150 */
	metersim_getEnergyTariff(mctx, energy, 2);

	metersim_free(mctx);
```

### Simulator with runner
Here simulator is equipped with a runner mocking the time lapse (that can possibly be sped up).

See the following example:
```c
	int phases, currentTariff;
	metersim_energy_t energy[3];

	/* Allocate memory for the simulator */
	mctx = metersim_init("input_dir"); /* Pass scenario directory */
	if (mctx == NULL) {
		printf("Unable to create metersim instance.\n");
		return EXIT_FAILURE;
	}

	/* Create runner, but do not start it */
	metersim_createRunner(mctx, 0);

	/* Set speedup */
	metersim_setSpeedup(mctx, 10);

	/* Get the number of phases */
	metersim_getPhaseCount(mctx, &phases);

	/* Schedule a pause at timestamp 3600 */
	metersim_pause(mctx, 3600);

	/* Start runner */
	metersim_resume(mctx);

	sleep(4);

	metersim_getTariffCurrent(mctx, &currentTariff);
	metersim_getEnergyTariff(mctx, energy, currentTariff);

	/* ... */

	metersim_destroyRunner(mctx);
	metersim_free(mctx);
```
Take a look at `examples/simulate_metersim/simulate_metersim.c` for a full example.

### Intertwining of the calculator and the runner
The above two paradigms (calculator and runner) can be successfully combined in an application. The only constraint is that `metersim_stepForward` can be used only when the runner is paused (otherwise it performs no changes and returns `METERSIM_ERROR`).

```c
	metersim_energy_t energy;

	/* Allocate memory for the simulator */
	mctx = metersim_init("input_dir"); /* Pass scenario directory */
	if (mctx == NULL) {
		printf("Unable to create metersim instance.\n");
		return EXIT_FAILURE;
	}

	metersim_createRunner(mctx, 0);
	metersim_setSpeedup(mctx, 10);
	metersim_pause(mctx, 500);
	metersim_resume(mctx);

	while (metersim_isRunning(mctx) == 1) {
		metersim_getEnergyTotal(mctx, &energy);
		printf("Total active+ energy: %ld\n", energy.activePlus.value);
		sleep(1);
	}
	/* Now runner is paused and timestamp is 500 */

	metersim_stepForward(mctx, 500);

	/* Total energy at timestamp 1000 */
	metersim_getEnergyTotal(mctx, &energy);
	printf("Total active+: %ld\n", energy.activePlus.value);

	metersim_resume(mctx);

	/* ... */

	metersim_destroyRunner(mctx);
	metersim_free(mctx);
```

### Devices

SEM Simulator can be equipped with some devices that dynamically change the current flow.

Each time the state of the Simulator changes, the devices are asked for the current they generate. The device is understood as a callback of the form:
```c
void (*callback)(metersim_infoForDevice_t *info, metersim_deviceResponse_t *res, void *ctx)
```

Here `ctx` is set by the user and provides user data for the callback. Also, `info` and `res` are input and output of the callback, respectively. Their types are as follows:
```c
typedef struct {
	double _Complex voltage[3];  /* Voltage provided by the grid */
	int32_t now;                 /* Current monotonic timestamp (uptime) */
	int64_t nowUtc;
} metersim_infoForDevice_t;

typedef struct {
	double _Complex current[3];  /* Current produced by the device */
	int32_t nextUpdateTime;      /* Timestamp of the next scheduled device's change of state */
} metersim_deviceResponse_t;
```

If the state of the devices changes not accordingly to schedule and one wants to notify the Simulator, so that it polls the device, one should use the function:
```c
void metersim_notifyDevicemgr(metersim_ctx_t *ctx);
```

See the following example:

```c
static void cb1(metersim_infoForDevice_t *info, metersim_deviceResponse_t *res, void *ctx)
{
	(void)ctx;
	int32_t now = info->now;
	if (now < 60) {
		res->current[0] = now;
		res->current[1] = now * cexp(225.0 * M_PI / 180.0 * _Complex_I);
		res->current[2] = now * cexp(275.0 * M_PI / 180.0 * _Complex_I);
		res->nextUpdateTime = now + 1;
	}
	else {
		res->nextUpdateTime = -1;
	}
}


typedef struct {
	int switchOn;
	pthread_mutex_t lock;
} ctx2_t;


static void cb2(metersim_infoForDevice_t *info, metersim_deviceResponse_t *res, void *ctx)
{
	(void)info;
	ctx2_t *ctx2 = (ctx2_t *)ctx;

	pthread_mutex_lock(&ctx2->lock);
	if (ctx2->switchOn == 1) {
		res->current[0] = 10;
	}
	else {
		res->current[0] = -15;
	}
	pthread_mutex_unlock(&ctx2->lock);
	res->nextUpdateTime = -1;
}


int main(void)
{
	metersim_deviceCtx_t *dev1;
	metersim_deviceCtx_t *dev2;

	ctx2_t cbCtx;

	status = pthread_mutex_init(&cbCtx.lock, NULL);
	TEST_ASSERT(status == 0);
	if (status < 0) {
		return;
	}

	cbCtx.switchOn = 1;

	metersim_ctx_t ctx = metersim_init("scenarioDir");

	dev1 = metersim_newDevice(ctx, &cb1, NULL);

	dev2 = metersim_newDevice(ctx, &cb2, &cbCtx);

	metersim_createRunner(ctx, True);

	/* ... */

	sleep(5);

	pthread_mutex_lock(&cbCtx.lock);
	cbCtx.switchOn = 0;
	pthread_mutex_unlock(&cbCtx.lock);
	metersim_notifyDeviceMgr(ctx);

	/* ... */

	sleep(15);

	pthread_mutex_lock(&cbCtx.lock);
	cbCtx.switchOn = 1;
	pthread_mutex_unlock(&cbCtx.lock);

	metersim_notifyDeviceMgr(ctx);

	/* ... */
}
```


## Usage of the `mm_api` (meter-message API)
This API is designed for the user applications running on an actual Smart Energy Meter. The API functions and types are defined in `include/mm_api.h`.

Since this API is not designed to run simulations, there is a couple of simulation-specific things one should note:

* Setting the input directory: the directory `inputDir` containing config files should be set in place of the address of the server.

```c
mm_ctx_T *mctx = NULL;
mctx = mm_init();
if (mctx == NULL) {
	printf("Unable to create mm instance");
	return 1;
}

struct mm_address addr = {
	.addr = "inputDir", /* Passing scenario directory as address */
	.typ = 1
};

mm_connect(mctx, &addr, NULL);

/* ... */
```
* The simulation starts when `mm_connect` is invoked.
* The simulation finishes when `mm_disconnect` is invoked.

Take a look at `examples/simulate_mm_api/simulate_mm_api.c` for a full example.

## Examples
After building the project in directory `build` one can run the examples:

#### Simulation with `metersim` API
The example shows how to properly initialize, configure and manipulate the simulation. It also illustrates how to get the values of various registers during the simulation.

To run the simulation call
```
./build/examples/simulate_metersim examples/simulate_metersim/scenario
```

#### Simulation with `metersim` API
This example shows how to run the simulation with `mm_api`. It illustrates how to initialize and configure it, as well as how to acquire metrology data during the simulation.

To run the simulation call
```
./build/examples/simulate_mm_api examples/simulate_mm_api/scenario
```

## Unit tests
After building the project one can run unit tests with the script `run_tests.sh`. Input files for unit tests are defined in directory `test/input`.

NOTE: By default, the tests assume that the build directory is `build`, but it can be specified by:
```bash
BUILD_DIR=some_directory ./run_test.sh
```


## Python module

In Python we provide both `metersim` and `mm_api` APIs. See files `mm_api.py` and `metersim.py` in directory `python_api/phoenixsystems/sem`.

The Python API is just a wrapper on the API in C, so all the functions work exactly the same.

Examples are provided in `example/simulate_metersim/simulate_metersim.py` and `examples/simulate_mm_api/simulate_mm_api.py`.

### Example: Simple simulation
Prepare virtual env:

```bash
python3 python_api/prepare_venv.py
source _venv/bin/activate
```

Run one of the simple simulation examples below. These examples have exactly the same structure as their counterparts in C.

```bash
#Simulation with metersim API
python3 examples/simulate_metersim/simulate_metersim.py

#Simulation with mm_api API
python3 examples/simulate_mm_api/simulate_mm_api.py
```

### Devices
In Python the idea of the devices is the same, but the realisation is slightly different. We provide the base class:

```Python
class Device(ABC):
    mgr: Any

    def notify(self) -> None:
        """
		Notifies the Simulator that the state has changed
		and it should poll the device for updates
		"""
        self.mgr.notify()

    def getTime(self) -> int:
        """Get current timestamp from the Simulator"""
        return self.mgr.getTime()

    @abstractmethod
    def update(self, info: InfoForDevice) -> DeviceResponse:
        """The device callback"""
        pass
```

NOTE: One should not use `self.getTime()` or `self.notify()` inside the `self.update` function, as this will result in a deadlock. And one should not be tempted to do so, as current timestamp is provided in `self.update` function as `info.now` and there should not be any reason for notifying the Simulator, since the device is currently being updated.


The `metersim.Metersim` class provides a method for adding devices.
```Python
def addDevice(self, device: Device) -> None:
```

Here is an example of a device object:
```Python
class MyDev(Device):
    switchOn: bool = False

    def changeState(self, state: bool):
        oldState = self.switchOn
        self.switchOn = state
        if self.switchOn != oldState:
            self.notify()

    def update(self, info: InfoForDevice) -> DeviceResponse:
        res = DeviceResponse([0, 0, 0], -1)
        if self.switchOn == 1:
            res.current[0] = 10
        else:
            res.current[0] = -15
        return res
```

Classes `InfoForDevice` and `DeviceResponse` are defined in `api_py/phoenixsystems/sem/device.py`.
