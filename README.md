# Anti-Freeze

ESP32 based controller to periodically run the hot water baseboard heat during 
extremely cold periods to prevent possible freezing of the water pipes.

# Background

Air source mini-split heat pumps have supplanted our oil fired baseboard heat 
system.  On very cold nights it is _possible_ that parts of the pipes supplying 
the base board heat system may now freeze since they are full of water but not 
being used. To insure against this I developed this system to periodically 
circulate hot water through the baseboard system during extremely cold periods.

# Design  

NOTE: This design is for a two wire control, where the thermostat calls for heat 
by closing the switch on the thermostat wire circuit.

1. This system is fail-safe: it runs in parallel to, and independently of, the 
   existing baseboard thermostat and mini-split system.
1. The existing thermostat wire is spliced to run, in parallel, to a 28 VAC 
   rated, opto-coupled relay activated by the ESP32. 
1. A DS18B20 temperature sensor is used to measure outdoor temperatures.
1. When the outdoor temperature `T` falls below `T_freeze_danger` the relay is
   activated for one minute every `M` minutes. 
1. `M = 60 / (1 + k * (T_freeze_danger - T))`. By default `k=0.1`
1. The device keeps a log of the temperatures measured and relay activations.
1. The onboard LED shows a heartbeat every 10 seconds when `T` is more than 
   `T_freeze_danger`.
1. The onboard LED shows a heartbeat every 2 seconds when `T` is less than 
   `T_freeze_danger`.  
1. The onboard LED flashes every 500ms when the relay is activated

**The following services/features are available if the device can connect to the 
home router** 
1. The device is discoverable via mDNS as `antifreeze.local` with a service type 
   of `antifreeze.tcp`
1. It serves a single webpage that has a form to set (and show) 
   `T_freeze_danger` and `k` which can be adjusted.
1. The webpage also shows the event log: temperature measured and relay 
   activations
1. The device gets it's time from an ntp server and log time stamps are based on 
   UTC rather than elapsed time since boot. 

# Hardware connections

```
[ Boiler relay box ]===========++==============[ Thermostat ]
                               ||
                               ||
                               ++==========[ Relay ]----- GPIO 33

[DS18B20] ----------------------------------------------- GPIO 15
```

# To build and flash

1. `idf.py add-dependency espressif/mdns` ([ESP mDNS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/mdns.html))
1. `idf.py flash`


# Techniques demonstrated

1. Digital output
1. RTOS multi-tasking
1. WiFi connection
1. mDNS
1. Http server
1. Getting time via SNTP
1. Dallas one wire protocol

# Appendix

## Halt on error configuration
(Found in: [Component config > ESP System Settings](panic))

[panic]: https://docs.espressif.com/projects/esp-idf/en/release-v5.2/esp32/api-reference/kconfig.html#config-esp-system-panic

**This should be set to reboot to avoid the possibility that the relay gets stuck in the on position. It also takes care of any transient faults that might have crashed the application.**

## Ideas for sensing boiler relay activation

1. ZMPT101B Voltage Transformer Voltage Sensor 
1. HCPL3700