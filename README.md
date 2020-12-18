# failsafe_dlux
 Failsafe for Turnigy dlux 250A HV 14s 60v ESC

Issue: The ESC has a hold time of almost 3 seconds in case of PPM signal loss. The PPM signal loss may occur if RX power supply fails (or in case of reception issues without RX-failsafe).

Solution approach: The circuit is powered by the RX. The supply is decoupled by a schottky diode and buffered by a capacitor. In case of RX power failure the circuit sets the ESC to the predefined failsafe position (failsafe_pos). Power failure is detected by comparing the supply in front of the diode with the predefined limit (voltagelimit). 
Failsafe is also triggered if the last proper PPM signal is older than the predefined limit (delaylimit).

red LED: indicates if output signal is below or equal failsafe position (either due to corresponding input signal or activated failsafe)
Blink code: 1 blink = PPM signal loss occurred at least once, 2 blínks = supply voltage was below limit at least once

demonstration:
https://www.youtube.com/watch?v=K2Zalw2rsIU

old version history

V.0.9: 
- first try

V.1.0: 
- first stable version
- improved software structure
- blink code for errors
- solution for delay control that was challenged by interrupt 

V.1.0.1:
- adapted for nano_RC_hold_V1.0 PCBs (pin number for input and output); no change needed for nano_RC_hold_V1.1 PCBs

V.1.0.2:
- using map function to scale ADC voltage to input voltage
- implementation 10k/68k voltage divider for 8.4V input voltage
- implementation of pistole grip stick conversion (pistol grip transmitter rest in neutral position, many ESC require -100% signal to stop)

2020/12/18 moved to github


