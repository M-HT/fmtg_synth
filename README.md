# FMTG Synth

FMTG Synth is a [software synthesizer](https://en.wikipedia.org/wiki/Software_synthesizer) based on [FM-tone-generator](https://github.com/kwhr0/FM-tone-generator) project (8-operator FM tone generator).

This project uses FM-tone-generator project to build some tools (like Linux ALSA driver).


The source code is released with [MIT license](https://spdx.org/licenses/MIT.html).

<hr/>

The projects consists of following parts:

* **FM-tone-generator**
  * FM-tone-generator project submodule
  * Either clone this project with `--recurse-submodules` parameter or run `git submodule init` followed by `git submodule update` after cloning this project.
* **src**
  * Source code to create following programs:
  * *fmtg_alsadrv* - Linux daemon which provides [ALSA](https://en.wikipedia.org/wiki/Advanced_Linux_Sound_Architecture) MIDI sequencer interface.
  * *fmtg_pcmconvert* - Tool to convert [Standard MIDI File](https://www.midi.org/specifications-old/item/standard-midi-files-smf) to *PCM* (*WAV* or *RAW*).
  * Both programs require a datafile to work.
* **datafiles**
  * *tone1.bin* - Datafile based on [htsfms](https://web.archive.org/web/20220102090815/http://hp.vector.co.jp/authors/VA024632/) 0.7
  * *tone2.bin* - Datafile based on htsfms 0.9
  * *tone3.bin* - Datafile based on htsfms 0.5
  * Warning: Not all features of htsfms are supported by FM-tone-generator.
* **documentation**
  * MIDI implementation
