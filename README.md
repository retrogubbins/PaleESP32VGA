# PaleESP32VGA
Emulation of Camputers Lynx 96K + Disks on TTGO VGA ESP32

Works on the Rev 1.4 board using PSRAM for disk support (will work without this)

This is an emulation of the Camputers Lynx 96K Computer from 1983

it includes very basic disk support:

XROM starts the DOS
EXT DIR  and   EXT  LOAD/MLOAD  work

F6 cycles round different JDx.LDF disk images held on SD card

F2 toggles fast/slow speed

F11 reboots ESP32  (needed to reenable disk support)
F12 reboots Z80
