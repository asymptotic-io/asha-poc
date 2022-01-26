# Hearing aid support over BLE

This is a proof of concept implementation of the ASHA protocol over BlueZ. It's
a precursor to the final goal of having an implementation of the ASHA spec
within Bluez.

### Building

Currently has 3 dev dependencies that need to be available: pthread, dbus and bluez. To build, run:

```
$ meson build
$ cd build
$ ninja
```

### Running

To run, you will have to start bluetoothd with the following configuration set

```
[LE]

MinConnectionInterval=16
MaxConnectionInterval=16
```

This is necessary for audio to reach the hearing aid on time and is explained in more detail in the docs.

To execute, run it with the Bluetooth device address with the octets separated
by underscores as the first argument.

```
$ ./asha-support 71_D3_BF_2F_9A_A0
```

Currently, there is a bug where the first run may end abruptly because the
service interface may not have been exposed on time. You will have to run it
twice to succeed.

The current implementation streams a part of an audio file that is checked into
the codebase. This behaviour is hardcoded.

### How it works

This project provides an implementation of the
[ASHA](https://source.android.com/devices/bluetooth/asha) specification. The
specification describes a combination of GATT services, L2CAP connections,
audio encodings and rules around how to use them. Not all of which could be
implemented here. Some of this is described in more detail in the docs.
