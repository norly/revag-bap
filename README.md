VAG / VW reverse-engineered BAP (Bedien- und Anzeigeprotokoll) implementation
==============================================================================

This is a dump of my previous work in re-implementing the BAP (Bedien-
und Anzeigeprotokoll) used on the CAN bus in a VW Golf Mk 6, using
Linux SocketCAN.

It contains tools to dump, decode, and somewhat intelligently
sniff BAP channels. Sending is not really done yet.

The code is far from neat and finished, but I'd rather have it out
in the open than lost in the sands of time.


Sources
--------

The vendor specific CAN protocol has been reverse engineered entirely
from wire traces between an RCD 310 head unit and a MDI device.


Acknowledgements
-----------------

Special thanks go out to tmbinc who inspired me to do this, and
laid out the simplicity of BAP in his talk on hacking his handsfree
unit in his Golf Mk 6:

  https://media.ccc.de/v/30C3_-_5360_-_en_-_saal_2_-_201312281600_-_script_your_car_-_felix_tmbinc_domke
  https://github.com/tmbinc/car
  https://github.com/tmbinc/kisim/


Disclaimer
-----------

This code has only been used in a lab bench setup, driving an RCD 310
radio head unit, as well as a MDI/Media-In interface. It has NOT been
tested inside a real car, and the author(s) take NO responsibility
whatsoever for any damage, safety issues, or anything else, be it
in a lab setup or in an actual car.


Licence
--------

GNU GPL v2 only.

Please see the file COPYING for details.
