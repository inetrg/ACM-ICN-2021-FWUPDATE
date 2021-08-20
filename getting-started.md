# Getting Started

## Prerequisites
The applications to reproduce our comparative evaluations run on the [RIOT](https://github.com/RIOT-OS/RIOT) operating system.
The [RIOT tutorial](https://github.com/RIOT-OS/Tutorials) provides a setup guide for installing the necessary toolchains.
More information on how to compile an application for RIOT and on how to flash the resulting binary on real hardware is summarized in [The Quickest Start](https://doc.riot-os.org/index.html#the-quickest-start) and the [Getting Started](https://doc.riot-os.org/getting-started.html).

The experiments are designed to run on the [FIT IoT-Lab testbed](https://www.iot-lab.info/).
All protocol deployments use the [IoT-Lab M3](https://www.iot-lab.info/docs/boards/iot-lab-m3/) board.

More information on the `m3` nodes sum up [here](https://doc.riot-os.org/group__boards__iotlab-m3.html) and [here](https://iot-lab.github.io/docs/boards/iot-lab-m3/).

Follow [this guide](https://www.iot-lab.info/legacy/tutorials/getting-started-tutorial/index.html) for a step-wise description on scheduling experiments in the testbed.

## Code
The [`ndn/app`](ndn/app/) folder contains the application that we used to perform our experiments.

## Clone Repository
The explicit RIOT version is included as a git submodule.
This repository needs to be cloned recursively with:

```
git clone --recursive https://github.com/inetrg/ACM-ICN-2021-FWUPDATE.git
```

If the `RIOT submodule` is empty, then update the submodule.
From the root of this repository, run:

```
git submodule update --init
```

## Compile Application

To compile a particular application, we first change into the app directory:

```
cd ndn/app
```

and then compile for the `m3` board using the following command:

```
BOARD=iotlab-m3 make all
```

This generates the binary files `bin/app.[bin,elf]`, which can then be flashed on the testbed nodes following the step-wise [testbed guide](https://www.iot-lab.info/legacy/tutorials/getting-started-tutorial/index.html).

For this experiment, the `manage.sh` script sets required environment variables, processes static forwarding entries, and prepares the binary for experimentation.
