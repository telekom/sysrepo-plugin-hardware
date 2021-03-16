# DT IETF Hardware Plugin

A sysrepo plugin for the IETF-Hardware YANG module [RFC8348](https://tools.ietf.org/html/rfc8348)

## Building

The plugin is built as a shared library using the [MESON build system](https://mesonbuild.com/) and is required for building the plugin.

```bash
apt install meson ninja-build cmake
```

The plugin install location is the `{prefix}` folder, the `libietf-hardware-plugin.so` should be copied over to the `plugins` folder from the sysrepo installation\
Build by making a build directory (i.e. build/), run meson in that dir, and then use ninja to build the desired target.

```bash
mkdir /opt/dt-ietf-hardware
meson -D prefix="/opt/dt-ietf-hardware" ./build
ninja -C ./build
```

## Installing

Meson installs the shared-library in the `{prefix}` and the .yang files necessary for the plugin functionality under {prefix}/yang

```bash
meson install -C ./build
```

## Running and testing the plugin
The necessary .yang files need to be installed in sysrepo from `{prefix}/yang`:

```bash
sysrepoctl -i /opt/dt-ietf-hardware/yang/iana-hardware.yang
sysrepoctl -i /opt/dt-ietf-hardware/yang/ietf-hardware.yang
sysrepoctl -c ietf-hardware -e hardware-sensor
```

The sysrepo plugin daemon needs to be loaded after the plugin is installed

```bash
sysrepo-plugind -v 4 -d
```

The functionality can be tested doing an operational data request through sysrepo, only the root-node is currently supported for data-requests:

```bash
sysrepocfg -x "/ietf-hardware:hardware" -X -d operational -f json
```

### Dependencies
```
libyang
libsysrepo
libsensors
```

```bash
sudo apt install lm-sensors libsensors-dev
```

### Nodes that are currently implemented
DONE - nodes are implemented and a value is provided if such information can be retrieved\
NA - node is not implemented because the value can't be found in Debian systems\
IN PROGRESS - values can be partly available\
NOT IMPLEMENTED - values are not yet provided, could be in the future

```
module: ietf-hardware
  +--rw hardware
     +--ro last-change?   yang:date-and-time                          DONE
     +--rw component* [name]                                          DONE
        +--rw name              string                                DONE
        +--rw class             identityref                           DONE
        +--ro physical-index?   int32 {entity-mib}?                   NA (feature not available)
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
        +--rw asset-id?         string                                NA
        +--ro is-fru?           boolean                               NA
        +--ro mfg-date?         yang:date-and-time                    NA
        +--rw uri*              inet:uri                              NA
        +--ro uuid?             yang:uuid                             DONE
        +--rw state {hardware-state}?                                 NOT IMPLEMENTED
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

  notifications:                                                      NOT IMPLEMENTED
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
