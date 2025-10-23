# MIDI Implementation

## Channel Voice Messages

|MIDI Message|HEX Code|Description|Compatibility|
|:---------- |:------ |:--------- |:-----------:|
|NOTE ON|9nH kk vv|Midi channel n(0-15) note ON #kk(0-127), velocity vv(1-127).<br>vv=0 means NOTE OFF|MIDI|
|NOTE OFF|8nH kk vv|Midi channel n(0-15) note OFF #kk(0-127), vv is ignored.|MIDI|
|PITCH BEND|EnH bl bh|Pitch bend as specified by bh\|bl (14 bits).<br>Maximum swing is +/- 1 tone (power-up).<br>Can be changed using « Pitch bend sensitivity ».<br>Center position is 00H 40H.|GM|
|PROGRAM<br>CHANGE|CnH pp|Program (patch) change.<br>Specific action on channel 10 (n=9) : select drumset.|GM/GS|
|CTRL 06|BnH 06H cc|Data entry MSB : provides data to RPN and NRPN|MIDI|
|CTRL 07|BnH 07H cc|Volume|MIDI|
|CTRL 10|BnH 0AH cc|Pan|MIDI|
|CTRL 11|BnH 0BH cc|Expression|MIDI/GM|
|CTRL 98|BnH 62H vv|NRPN LSB|MIDI|
|CTRL 99|BnH 63H vv|NRPN MSB|MIDI|
|CTRL 100|BnH 64H vv|RPN LSB|MIDI|
|CTRL 101|BnH 65H vv|RPN MSB|MIDI|
|CTRL 120|BnH 78H xx|All sounds off (abrupt stop of sound on channel n)|MIDI|
|CTRL 121|BnH 79H xx|Reset all controllers|MIDI|
|CTRL 123|BnH 7BH xx|All notes off|MIDI|
|CTRL 124|BnH 7CH xx|Omni off.<br>The same processing will be carried out as when « All Sounds Off » is received.|MIDI|
|CTRL 125|BnH 7DH xx|Omni on.<br>The same processing will be carried out as when « All Sounds Off » is received.|MIDI|
|CTRL 126|BnH 7EH xx|Mono on.<br>The same processing will be carried out as when « All Sounds Off » is received.|MIDI|
|CTRL 127|BnH 7FH xx|Poly on.<br>The same processing will be carried out as when « All Sounds Off » is received.|MIDI|
|RPN 0000H|BnH 65H 00H 64H 00H 06H vv|Pitch bend sensitivity in semitones #vv(0-31)|MIDI/GM|

## System Exclusive Messages

|Manufacturer ID|HEX Code|Description|Compatibility|
|:------------- |:------ |:--------- |:-----------:|
|Universal Non-Real Time|F0H 7EH 7FH 09H 01H F7H|General MIDI reset|GM|
|Yamaha Corporation|F0H 43H 1xH 4CH 00H 00H 7EH xx F7|XG reset<br>The same processing will be carried out as when « GM reset » is received.|XG|
|Yamaha Corporation|F0H 43H 1xH 4CH 08H 0pH 07H vv F7|Part mode, p is part (0 to 15), vv is 00 (normal part) or 01 (drum part).|XG|
|Roland Corporation|F0H 41H xx 42H 12H 00H 00H 7FH vv xx F7H|System mode set, vv is 00 (Mode-1: Single module mode) or 01 (Mode-2: Double module mode).<br>The same processing will be carried out as when « GM reset » is received.|GS|
|Roland Corporation|F0H 41H xx 42H 12H 40H 00H 7FH 00H xx F7H|GS reset<br>The same processing will be carried out as when « GM reset » is received.|GS|
|Roland Corporation|F0H 41H xx 42H 12H 40H 1pH 15H vv xx F7H|Part to rhythm allocation, p is part (0 to 15), vv is 00 (sound part) or 01 (rhythm part).|GS|

