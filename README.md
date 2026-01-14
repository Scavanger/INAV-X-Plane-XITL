# INAV X-Plane XITL plugin

## XITL - HITL (Hardware) and SITL (Software) in the loop for INAV


Since the original X-Plane HITL plugin appears to be no longer in development, we are continuing here.
Name change to XITL to make it clear that HITL and SITL connections are supported equally.

New features and improvements in version 2.0:

#### OSD
- New shader-based rendering to prevent artifacts
- Support for the latest “side-by-side” HD fonts
- Current hi-res fonts from [SneakyFPV](https://sites.google.com/view/sneaky-fpv) for DJI and Avatar OSD
- The OSD type is automatically detected and fonts can be set separately for each system
  
#### UI
- New, clearer menu and new settings dialog
- Voice output replaced with on-screen messages
- HD font as standard for on-screen messages

#### SITL/HITL
- Rangefinder sensor with 10 m range
- Simulation of a drive train (Lipo/Lion battery with different capacities and motor/propeller for realistic voltage and current)
- Simulation of the reception strength (RSSI) of a virtual receiver
- Failsafe simulation

#### HITL
- Option to use the USB transmitter/joystick connected to X-Plane
- Option to set the serial port manually
- Linux: Connection can also be established via `/dev/USBX` devices (“real” USB-serial adapters).
  
#### SITL
- Communication with SITL revised to avoid setting values twice 

#### General
- Add option to reboot INAV
- Internal code refactoring

### INAV
To use the new features, changes to INAV are necessary. Until the changes are included in the official INAV release, a patched INAV 9 version can be downloaded here: https://github.com/Scavanger/INAV-XITL-Firmware

### MacOS
Unfortunately, I don't have a Mac, so I can't test the plugin here, which means there won't be a release for the time being.

If anyone feels up to the task, they are welcome to create a pull request. The code already contains defines and a cmake file for Mac, but as I said, it is untested. 

## General

**Hardware/Software-in-the-loop** plugin for **X-Plane 11 & 12** for **INAV Flight Controller firmware**: 

**Hardware/Software-in-the-loop (HITL/SITL) simulation**, is a technique that is used in the development and testing of complex real-time embedded systems. https://github.com/Scavanger/INAV-XITL-Firmware
d testing of complex real-time embedded systems. 
**X-Plane** is a flight simulation engine series developed and published by Laminar Research https://www.x-plane.com/

**INAV X-Plane XITL** is plugin for **X-Plane** for testing and developing **INAV flight controller firmware** https://github.com/iNavFlight/inav.


## Open to collaboration

Anyone who has ideas, bug fixes, or features is welcome to collaborate and create pull requests.

## Motivation

I believe that good testing and debugging tools are key points to achieve software stability.

It is not Ok when people debug autopilot by running with RC Plane on the field :smiley:

I hope this plugin can help to improve INAV firmware.

While not 
**Hardware/Software-in-the-loop** plugin for **X-Plane 11 & 12** for **INAV Flight Controller firmware**: 

**Hardware/Software-in-the-loop (HITL/SITL) simulation**, is a technique that is used in the development and testing of complex real-time embedded systems. 

**X-Plane** is a flight simulation engine series developed and published by Laminar Research https://www.x-plane.com/

**INAV X-Plane XITL** is plugin for **X-Plane** for testing and developing **INAV been a main purpose, plugin can be used to improve pilot skils or getting familiar with INAV settings.

#
**Hardware/Software-in-the-loop** plugin for **X-Plane 11 & 12** for **INAV Flight Controller firmware**: 

**Hardware/Software-in-the-loop (HITL/SITL) simulation**, is a technique that is used in the development and testing of complex real-time embedded systems. 

**X-Plane** is a flight simulation engine series developed and published by Laminar Research https://www.x-plane.com/

**INAV X-Plane XITL** is plugin for **X-Plane** for testing and developing **INAV ![](doc/img/x-plane-logo.png) 

**X-Plane** https://www.x-plane.com/ is flight simulator with accurate physics simulation. 
 
X-Plane is extendable with plugins. This plugin connects to Flight Controller through USB cable and passes gyroscope, accelerometer, barometer, magnethometer, GPS and Pitot data from X-Plane to FC. Simulated sensors data replaces readings from physical sensors. 

FC sends back **yaw/pitch/roll/trottle** controls which are passed to X-Plane.

## X-Plane 11 or X-Plane 12?

Simulation requires at least 50, or even better 100, solid FPS without freezing. 

While **X-Plane 12** has better visual appearance, **X-Plane 11** is still recommented choice due to better performance. 

Also, small aircraft physics seems to work better in X-Plane 11.

### X-Plane 11 and modern Linux distributions

X-Plane 11 (or rather the network subsystem) no longer works on modern Linux systems with kernel > 6.9: https://www.x-plane.com/kb/fixing-x-plane-11-getting-stuck-on-will-init-net-on-linux/

This breaks the XITL plugin. A patch/workaround can be found here: https://github.com/datafl4sh/xpfix

If you cannot or do not want to compile it yourself, here is a pre-compiled version: [xpfix.so](/doc/extra/xpfix.so)

# Setup and usage

See [setup.md](doc/setup.md)

# Development

See [development.md](doc/development.md)

# Special thanks

Many thanks to:
- Sergii Sevriugin for initial implementation and a lot of testing
- Roman Lut, original Autor
- SneakyFPV for HD fonts
- NKDesign for NK FPV SurfWing RC plane for X-Plane
- b14ckyy for Surfwing 3D model
- Bart Slinger for MacOs plugin compilation


# Links

- X-Plane INAV HITL prototype has been orignally implemented by Sergii Sevriugin: 

   https://github.com/sevrugin/inav/tree/master-simulator-xplane

   https://github.com/sevrugin/inav-configurator/tree/master-simulator-xplane

   [![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/XeYr-l9Sowc/0.jpg)](https://www.youtube.com/watch?v=XeYr-l9Sowc)

- NK_FPV Surfwing V2 | RC Plane 2.2.0 

   https://forums.x-plane.org/index.php?/files/file/43974-nk_fpv-surfwing-v2-rc-plane/

- X PLANE TUTORIAL: MaxiSwift installation with X Plane v9.70 for HIL simulations 

   https://github.com/jlnaudin/x-drone/wiki/X-PLANE-TUTORIAL:-MaxiSwift-installation-with-X-Plane-v9.70-for-HIL-simulations

- Quadrotor UAV simulation modelling using X-Plane simulation software 

   http://www.iraj.in/journal/journal_file/journal_pdf/2-448-152361879882-85.pdf
