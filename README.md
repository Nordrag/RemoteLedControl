# RemoteLedControl
esp32 based hardware controller, it was used to operate a water pump. 
remote controll was achieved by polling an asp.net server
you could also directly controll the device if your phone was on the same wifi as the device
it was meant to report the water temperature, and toggle the water pump based on timer from the server

also this was my first C++ code, some probably terrible

things to check: 
state pattern: https://github.com/Nordrag/RemoteLedControl/blob/refactor/RemoteLedControl/StateMachine.h
main logic: https://github.com/Nordrag/RemoteLedControl/blob/refactor/RemoteLedControl/RemoteLedControl.ino
