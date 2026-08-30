`temper_query`
==============

Keep It Simple, Stupid, (KISS) userspace driver for TEMPer temperature and
humidity sensors.

[Releases](https://github.com/t-lo/temper-query/releases/) include self-contained static binaries w/o dependencies,
usable for embedded systems.

If you're rather looking for a full-featured stack, check out
[temper.py](https://github.com/ccwienk/temper).
Please note that `temper.py` is unrelated to this project.


Goals
-----

Built to support querying temperature and humidity from TEMPer sensors on a
embedded Linux environment (like the Raspberry Pi) with minimal dependencies
(only [HIDAPI](https://github.com/signal11/hidapi)).  Though untested, it should
be readily cross-platform amongst POSIX systems.

Most devices supported by
[`temper.py`](https://github.com/ccwienk/temper#supported-devicesa) are
supported, though few are tested.

Build
-----

`temper_query` can be built using the standard POSIX build procedure:
```bash
cd temper_query
mkdir build && cd build
../configure
make
```

The resulting executable can then be installed to the system:
```
sudo make install
```

### Static build in Docker container

To simplify static linked builds and cross-compilation, a wrapper build script `container-build.sh` is provided.
This script will start an ephemeral Alpine container, install dependencies, and statically compile `temper_query`.
It doesn't require any local dependencies (except `docker`).
Note that the script bypasses `configure` and `make`, and instead calls `gcc` directly.

### HIDAPI Dependency

`temper_query` depends on HIDAPI to query and receive the temperature response
from the TEMPer sensor.  It can be pulled from its
[GitHub](https://github.com/signal11/hidapi) repo and compiled from source or
downloaded in binary form via the appropriate system package manager.

Run
---

The `temper_query` can then be run (with elevated privileges to allow direct USB
access to the TEMPer sensor).
If successful, `termper_query` will print temperature and humidity (if
supported) of all devices found, one device per line.
```bash
sudo ./temper_query
/dev/hidraw2,TEMPerGold,TEMPerGold_V3.5 ,int-temp:26.43
/dev/hidraw4,TEMPerGold,TEMPerGold_V3.5 ,int-temp:27.25
```
The above example shows 2 `TEMPerGold_V3.5` temperature sensord.

Acknowledgements
----------------

This project is based on C. Ansel Horn's `temper_query`
https://github.com/cahorn/temper-query .

Many of the device definitions are taken from
[`temper.py`](https://github.com/ccwienk/temper#supported-devicesa).
