# FINAL PROJECT OVERVIEW

For my final project submission, I used a PD controller and an ESP32 to control a brushless DC motor. This README contains the hardware used by the system, the system electronics and wiring, and the software to drive the system. By the end of this README, you should build and replicate the results exaclty from this final project! At least, that is my hope with this README :)


## Tools Needed:
- Wire strippers
- Dupont wire crimper
- Dupont wire connectors, or Arduino jumper wires (a LOT, see wiring diagram)
- Soldering tool
- Solder
- [1] large C-clamp
- 3/16 allen wrench (I think)

## Hardware Requirements
- [1] ESP32-WROOM-32: https://a.co/d/05TtJUjU
- [1] USB-C data cable
- [1] Hoverboard Motor: https://a.co/d/0681CQAJ (DO NOT BUY THIS - this is just an example of the motor. Any 3-Phase, 120 degree out-of-phase motor with an included hall effect sensor *should* work)
- [1] RioRand 350W 6-60V 3-Phase PWM DC Brushless Motor Speed Controller with Hall Sensor: https://a.co/d/0fNlEnNE
- [1] Any 28V (or more) power supply will work, measured no-load power was ~10 Watts 
- [n] 1515 8020 aluminum extrusion (see attached CAD for exact details and dimensions, though these are not hard requirements)
- [3] 15 series 8020 Tall Gusseted Inside Corner Bracket: https://8020.net/4336.html
- [3] 15 series 8020 Lite Transition Inside Corner Bracket: https://8020.net/4509.html
- [20] 15 series 8020 Bolt Assembly: https://8020.net/3320.html
- [1] Breadboard or equivalent way of connecting circuitry together (see circuit diagram below, or here [TODO - attach link])
- [1] One-turn analog potentiometer
- [2] 2.2k Ohm resistors
- [1] basic on/off switch
- [20] WAGO Lever-Nuts 3-Conductors varient (or more, less probably won't work without being annoying): https://a.co/d/0gCFwZCQ
- [1] Taiss/Incremental Rotary Quadrature Encoder, 600P/R: https://a.co/d/0cLU70xH
- 3D printer or ability to 3D print 3 components
- [1] goBilda 1309 Series Sonic Hub (6mm D-Bore): https://www.gobilda.com/1309-series-sonic-hub-6mm-d-bore/
- [3] M3 x 8mm
- [4] M4 x 20mm
- [2] M4 x 12mm

## Software Requirements
- ESP-IDF v2.1.0 (I used the VScode plugin and followed instructions on getting it working directly from Espressif): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html
- Visual Studio Code
- Windows 11 (?)

## Software Configuration
In your project configuration, specifically in ```sdkconfig```, you must change the following parameters:

```CONFIG_FREERTOS_HZ=100```

Change this too:

```CONFIG_FREERTOS_HZ=1000```

# SETTING UP THE HARDWARE
## Step 1: print the hardware
The system relies on three 3D prints to connect the motor and the encoder. The motor doesn't have any traditional mounting holes since it was original designed to spin a tire really fast, and the tire was built into the rotor. As such, we need to print an adapter to allow us to connect the motor and the encoder. We also need to print a mount for the encoder.

### MOTOR SIDE PRINTS
The two prints that are attached to the motor are the armature and direction arrow. You can find them here [TODO - ADD LINK]

[ADD photos]

### ENCODER SIDE PRINTS
The only print needed for the encoder is it's mount, which you can find here: [TODO - ADD LINK]

[ADD photos]

### 8020 MOTOR MOUNT
Because this is a little complex and also not necessarily required (another mounting method can be designed), see the CAD for more detail [TODO - ADD LINK]

[TODO - ADD PICTURES OF THE CAD]

# SETTING UP THE ELECTRONICS
Below is an electrical schematic of the system. You can utilize the WAGO connectors for most connections, however you will need jumper wires for connecting to the ESP32 and other peripherals. The general implementation is up to interpretation, however the exact wiring of the system used for this project is reflected in the wiring diagram!

[TODO -ADD PICTURE OF WIRING DIAGRAM]

# SETTING UP THE SOFTWARE
After installing Visual Studio Code, ESP-IDF, and the VSCode ESP-IDF extension, you can then clone this repository, or whatever means you want to get the ESP32 project code onto your own machine
