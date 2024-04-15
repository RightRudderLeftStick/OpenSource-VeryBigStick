# Firmware for microcontrollers

## Breakout Board

**NOTE THE TM MINI DIN LAYOUT IS WRONG!!! I USED THE WRONG FOOTPRINT AGAIN SO IF YOU SOLDER THE MINI DIN DIRECTLY TO IT, IT WILL BE INCORRECT. LABELS ON THE BACK ARE OKAY IF YOU SOLDER NORMALLY!**

The breakout board is designed to socket either a STM32F103C8 microcontroller board (freejoy) or an Arduino Pro Micro (MMJOY2 or custom code). You can use the gerber file to order the PCB from your local assembler, or order directly from PCBWAY [here](https://www.pcbway.com/project/shareproject/Open_Source_Very_Large_Stick_Freejoy_MMjoy2_breakout_board_f66f472f.html).

![Screenshot](PCBBreakout.PNG)

## MMJOY2 version

The MMJOY2 variation will require an Andruino Pro Micro, and the solder bridge must be connected on the PCB to set it for this configuration.

This is the public directory of all the version of MMJOY2 that were released before the project was halted.

Link to [website.](https://sites.google.com/site/mmjoyproject/%D1%84%D0%B0%D0%B9%D0%BB%D1%8B-%D0%B4%D0%BB%D1%8F-%D1%81%D0%BA%D0%B0%D1%87%D0%B8%D0%B2%D0%B0%D0%BD%D0%B8%D1%8F) 

For best reliabilty and compatibility with the included .mmjoy2 file. Use MMJOY2[v20160818upd1].rar

## Freejoy version

Using the freejoy version requires a **genuine** STM32F103C8 microcontroller board. Counterfeit STM32 chips cause all sorts of unpredictable problems with the Freejoy firmware. Try and order your STM32F103C8 from a trusted retailer with a proper return policy. 

Freejoy version is still to come, and there will be a specialized firmware outside of what they currently distribute on the github to support the longer cable lengths required to connect the shift register. The breakout board revision 1.3 already supports all the required IO for freejoy.

Link to Freejoy [github.](https://github.com/FreeJoy-Team/FreeJoy)
