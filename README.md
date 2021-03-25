# DT IETF Hardware Plugin

A sysrepo plugin for the IETF-Hardware YANG module [RFC8348](https://tools.ietf.org/html/rfc8348)

## Building

The plugin is built as a shared library using the [MESON build system](https://mesonbuild.com/) and is required for building the plugin.

```bash
apt install meson ninja-build cmake pkg-config
```

The plugin install location is the `{prefix}` folder, the `libietf-hardware-plugin.so` should be copied over to the `plugins` folder from the sysrepo installation\
Build by making a build directory (i.e. build/), run meson in that dir, and then use ninja to build the desired target.

```bash
mkdir /opt/dt-ietf-hardware
meson -D prefix="/opt/dt-ietf-hardware" ./build
ninja -C ./build
```

## Installing

Meson installs the shared-library in the `{prefix}` and the .yang files necessary for the plugin functionality under `{prefix}/yang`.

```bash
ninja install -C ./build
mkdir -p /opt/sysrepo/lib/sysrepo/plugins
cp /opt/dt-ietf-hardware/libietf-hardware-plugin.so /opt/sysrepo/lib/sysrepo/plugins
```

## Running and testing the plugin
The necessary .yang files need to be installed in sysrepo from `{prefix}/yang`:

```bash
sysrepoctl -i /opt/dt-ietf-hardware/yang/iana-hardware.yang
sysrepoctl -i /opt/dt-ietf-hardware/yang/ietf-hardware.yang
sysrepoctl -c ietf-hardware -e hardware-sensor -e entity-mib
```

The sysrepo plugin daemon needs to be loaded after the plugin is installed:

```bash
sysrepo-plugind -v 4 -d
```

The functionality can be tested by doing an operational data request through sysrepo; data requests should be made for the `hardware`, `component` nodes:

```bash
sysrepocfg -x "/ietf-hardware:hardware" -X -d operational -f json
```

As described in the module itself the plugin holds configuration data provided by the user in the running data-store and uses the data according to the requirements of the RFC to change the hardware information provided in the operational data-store. A configuration example can be found under `yang/share` directory and can be imported in sysrepo using:

```bash
sysrepocfg -Idt-ietf-hardware-plugin-test-config.xml -d running
```

### Dependencies
```
libyang
sysrepo compiled with sysrepo-cpp
libsensors4-dev
lm-sensors
```

### Nodes that are currently implemented
DONE - nodes are implemented and a value is provided if such information can be retrieved\
NA - node is not implemented because the value can't be found in Debian systems

```
module: ietf-hardware
  +--rw hardware
     +--ro last-change?   yang:date-and-time                          DONE
     +--rw component* [name]                                          DONE
        +--rw name              string                                DONE
        +--rw class             identityref                           DONE
        +--ro physical-index?   int32 {entity-mib}?                   DONE
        +--ro description?      string                                DONE
        +--rw parent?           -> ../../component/name               DONE
        +--rw parent-rel-pos?   int32                                 DONE
        +--ro contains-child*   -> ../../component/name               DONE
        +--ro hardware-rev?     string                                DONE
        +--ro firmware-rev?     string                                DONE
        +--ro software-rev?     string                                DONE
        +--ro serial-num?       string                                DONE
        +--ro mfg-name?         string                                DONE
        +--ro model-name?       string                                DONE
        +--rw alias?            string                                DONE
        +--rw asset-id?         string                                DONE (only configuration side)
        +--ro is-fru?           boolean                               NA
        +--ro mfg-date?         yang:date-and-time                    NA
        +--rw uri*              inet:uri                              DONE (only configuration side)
        +--ro uuid?             yang:uuid                             DONE
        +--rw state {hardware-state}?                                 NA
        |  +--ro state-last-changed?   yang:date-and-time
        |  +--rw admin-state?          admin-state
        |  +--ro oper-state?           oper-state
        |  +--ro usage-state?          usage-state
        |  +--ro alarm-state?          alarm-state
        |  +--ro standby-state?        standby-state
        +--ro sensor-data {hardware-sensor}?                          DONE
           +--ro value?               sensor-value                    DONE
           +--ro value-type?          sensor-value-type               DONE
           +--ro value-scale?         sensor-value-scale              DONE
           +--ro value-precision?     sensor-value-precision          DONE
           +--ro oper-status?         sensor-status                   DONE
           +--ro units-display?       string                          DONE
           +--ro value-timestamp?     yang:date-and-time              DONE
           +--ro value-update-rate?   uint32                          DONE

  notifications:                                                      NA
    +---n hardware-state-change
    +---n hardware-state-oper-enabled {hardware-state}?
    |  +--ro name?          -> /hardware/component/name
    |  +--ro admin-state?   -> /hardware/component/state/admin-state
    |  +--ro alarm-state?   -> /hardware/component/state/alarm-state
    +---n hardware-state-oper-disabled {hardware-state}?
       +--ro name?          -> /hardware/component/name
       +--ro admin-state?   -> /hardware/component/state/admin-state
       +--ro alarm-state?   -> /hardware/component/state/alarm-state
```

### Assumptions made in the nodes value
The plugin uses `lshw` as the go-to tool for hardware information gathering, it has an option to output in json format and in this manner the information is collected by parsing the lshw output. By doing so a wide set of correlations have been made between the tool's output and the IETF-Hardware nodes, below you can find a map summarizing them:

```
IETF-Hardware node                                                     LSHW json node
----------------------------------------------------------------------------------------------------------------------
name (changed to '{parent}:{name} on collision')                       id (non-unique)
class (`unknown` where value isn't defined in IANA-Hardware)           class
physical-index (converted to int32)                                    physid (hex)
description                                                            description
parent                                                                 deducted as ascendant of `children` node
parent-rel-pos (increasing value, starting from 1)                     -
contains-child                                                         children/id (for every child)
hardware-rev                                                           version
firmware-rev                                                           children/id[firmware] OR configuration/firmware
software-rev                                                           configuration/driverversion
serial-num                                                             serial
mfg-name                                                               vendor
model-name                                                             product
alias                                                                  handle
asset-id                                                               -
is-fru                                                                 -
mfg-date                                                               -
uri                                                                    -
uuid                                                                   configuration/uuid
```
