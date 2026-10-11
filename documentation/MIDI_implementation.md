# MIDI Implementation

## Channel Voice Messages

|MIDI Message|HEX Code|Description|Compatibility|
|:-|:-|:-|:-:|
|NOTE ON|9nH kk vv|Midi channel n(0-15) note ON #kk(0-127), velocity vv(1-127).<br>vv=0 means NOTE OFF|MIDI|
|NOTE OFF|8nH kk xx|Midi channel n(0-15) note OFF #kk(0-127), xx is ignored.|MIDI|
|PITCH BEND|EnH bl bh|Pitch bend as specified by bh\|bl (14 bits).<br>Maximum swing is +/- 2 tone (power-up).<br>Can be changed using « Pitch bend sensitivity ».<br>Center position is 00H 40H.|GM|
|PROGRAM<br>CHANGE|CnH pp|Program (patch) change.<br>Specific action on channel 10 (n=9) : select drumset.|GM/GS|
|CTRL 06|BnH 06H cc|Data entry : provides data to RPN|MIDI|
|CTRL 07|BnH 07H cc|Volume|MIDI|
|CTRL 10|BnH 0AH cc|Pan|MIDI|
|CTRL 11|BnH 0BH cc|Expression|MIDI/GM|
|CTRL 98|BnH 62H xx|NRPN LSB|MIDI|
|CTRL 99|BnH 63H xx|NRPN MSB|MIDI|
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
|:-|:-|:-|:-:|
|Universal Non-Real Time|F0H 7EH 7FH 09H 01H F7H|General MIDI reset|GM|
|Kawai Musical Instruments MFG. Co. Ltd|F0h 40h 0xh 10h 00h 08h 00h 00h 00h 00h 00h F7h|Bank select: GM mode<br>The same processing will be carried out as when « GM reset » is received.|K11/GMega|
|Kawai Musical Instruments MFG. Co. Ltd|F0h 40h 0xh 10h 00h 08h 01h 00h 0ph 0mh 0lh F7h|Single #, p is part (0 to 15), m is 0-7 (sound part) or 8 (rhythm part).|K11/GMega|
|Kawai Musical Instruments MFG. Co. Ltd|F0h 40h 0xh 10h 00h 09h 03h 00h 0ph 0mh 0lh F7h|Single #, p is part (0 to 15), m is 0-7 (sound part) or A (rhythm part).|KC20|
|Roland Corporation|F0H 41H xx 42H 12H 00H 00H 7FH 0vh xx F7H|System mode set, vv is 0 (Mode-1: Single module mode) or 1 (Mode-2: Double module mode).<br>The same processing will be carried out as when « GM reset » is received.|GS|
|Roland Corporation|F0H 41H xx 42H 12H 40H 00H 7FH 00H xx F7H|GS reset<br>The same processing will be carried out as when « GM reset » is received.|GS|
|Roland Corporation|F0H 41H xx 42H 12H 40H 1pH 15H vv xx F7H|Part to rhythm allocation, p is part (0 to 15), vv is 00 (sound part) or 01 (rhythm part).|GS|
|Korg|F0H 42H 3xH 42H 12H 00H 00H 7Fh 00h F7|All parameter reset<br>The same processing will be carried out as when « GM reset » is received.|NS5R|
|Korg|F0H 42H 3xH 42H 12H 01H 0pH 0Ah vv F7|Part mode, p is part (0 to 15), vv is 00 (normal part) or 01-05 (drum part).|NS5R|
|Korg|F0H 42H 3xH 4CH 12H 00H 00H 7Fh 00h F7|All parameter reset<br>The same processing will be carried out as when « GM reset » is received.|N1|
|Korg|F0H 42H 3xH 4CH 12H 01H 0pH 0Ah vv F7|Part mode, p is part (0 to 15), vv is 00 (normal part) or 01-05 (drum part).|N1|
|Yamaha Corporation|F0H 43H 1xH 4CH 00H 00H 7EH xx F7|XG reset<br>The same processing will be carried out as when « GM reset » is received.|XG|
|Yamaha Corporation|F0H 43H 1xH 4CH 08H 0pH 07H vv F7|Part mode, p is part (0 to 15), vv is 00 (normal part) or 01 (drum part).|XG|
|Akai Electric Co. Ltd.|F0H 47H 10H 42H 5DH 40H 00H 7FH 00H F7|SG reset<br>The same processing will be carried out as when « GM reset » is received.|SG01|
|LG Electronics|F0H 00H 20H 23H 05H 0vH 0pH F7|Part mode, v is 2 (drum part) or 3 (normal part), p is part (0 to 15).|GS1000R|