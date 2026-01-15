# NK FPV Surfwing

 NK_FPV Surfwing V2 | RC Plane 2.2.0 https://forums.x-plane.org/index.php?/files/file/43974-nk_fpv-surfwing-v2-rc-plane/
 
![](/aircraft//NK_FPVSurfwing/NK_FPVSW_icon11_thumb.png)

A complete preconfigured example HITL configuration for INAV 9: [example-hitl-config.txt](example-hitl-config.txt) 

Modes are configured for the [INAV fixed wing group Open TX Pro Model](https://inavfixedwinggroup.com/guides/transmitter-models/pro-opentx-model-v2/)

## Recommended configuration

Set Airplane/Mixer type to "Airplane (with Tail)", even though it's a fixed-wing aircraft, X-Plane doesn't let us control the control surfaces directly, only roll, pitch and yaw.

For HITL and SITL.

INAV cli comands

### Mixer
```
set platform_type = AIRPLANE
set model_preview_type = 14

mmix 0  1.000  0.000  0.000  0.000

smix 0 1 1 100 0 -1
smix 1 2 0 100 0 -1
smix 2 3 0 100 0 -1
smix 3 4 2 100 0 -1

```

### Control profile (PIDs)

```
set fw_p_pitch = 15
set fw_i_pitch = 5
set fw_d_pitch = 5
set fw_ff_pitch = 180
set fw_p_roll = 15
set fw_i_roll = 3
set fw_d_roll = 7
set fw_ff_roll = 190
set fw_p_yaw = 50
set fw_i_yaw = 0
set fw_d_yaw = 20
set fw_ff_yaw = 255
set dterm_lpf_hz = 10
set fw_turn_assist_pitch_gain =  0.400
set nav_fw_pos_z_p = 25
set nav_fw_pos_z_d = 8
set nav_fw_pos_xy_p = 55
set d_boost_min =  1.000
set d_boost_max =  1.000
set rc_expo = 30
set rc_yaw_expo = 30
set roll_rate = 53
set pitch_rate = 50
set yaw_rate = 4
```

 Modifications applied to original model:
 - in order to prevent motor shutdown in inverted fight, an option has been changed to 1: "acf/_inverted_fuel_oil_EQ 1"
 - changed motor type to Electric
 - implemented custom motor sound script
 - changed 3D model
 - many other flying charactersitics adjustents were made


