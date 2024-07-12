### Testing

This application demonstrates a hang in RAIL library

To reproduce the issue you need

- At least three systems composed of a SiLabs dev base board and am EFR32ZG23B radio board.

- One of the dev boards should have a jumper on the expansion header between pin 20 (+3.3) and pin 13 (PA07).
  This tells the application to be the "root" node.  The other systems dont want a jumper
  
- Build and Flash this application to each board

- Observer the debug console output. Eventually one of the systems (usually the root) will stop scrolling
  and say "rx state timeout, aborting".  Attaching a debugger you will see the loop in RAIL polling some
  register in RAC forerver
  
That hang state happens when an RX SYNC DETECT is handled by the event callback, but no RX COMPLETION event
is gotten within a few seconds.  When the app detects this timeout it calls RAIL_Idle which hangs forever.

In the "real" application, the hang is usually as a result of calling RAIL_GetRadioStatus to determing if
the radio is in active RX to avoid TX during that time.

To build/flash

- Be in the directory levelmain

- Run the script `./builddkzg23.sh`

- Run the script `./flashdkzg23.sh` and select each devkit's id in turn


