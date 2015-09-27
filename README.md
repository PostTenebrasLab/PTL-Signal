# PTL-Signal2
Just like the Bat-Signal is used to call batman, the PTL-signal is used to call a PTL member to open the door!

When you're working in the machines room, it is very difficult to hear the phone because of the noise. So we made 2 arduino pro mini that communicates over RF using the famous nRF24L01 chips (with external antenna) and the CH1817 chip to detect phone rings.

When a phone ring is detected a message is sent to the 2nd arduino which opens a relay that starts flashing an industrial emergency light in the machines room! :)
