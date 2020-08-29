# Waves2 Oscillator

Waves2 is a custom user oscillator for minilogue xd and NTS-1.
It controls its sound's harmonics in a new way. 
You can download binaries from [the release page](https://github.com/boochow/waves2/releases).

## About

Waves2 is the oscillator which can control its harmonics WITHOUT filters. 
While Waves2 is a wavetable oscillator that uses pre-defined waveforms in the logue SDK -- the same as the pre-installed custom oscillator "Waves" --  it can produce new harmonics from original waveforms by "shifting up" the harmonics frequency distribution inside waveforms. 

## Demo

Waves2 alone

[![top-page](http://img.youtube.com/vi/EVoXINcKjwU/0.jpg)](https://youtu.be/EVoXINcKjwU)

Waves2 with VCO1 and VCO2

[![top-page](http://img.youtube.com/vi/EVoXINcKjwU/0.jpg)](https://youtu.be/K07AUfXWxt4)

## The Harmonics shifter

The main characteristic of this oscillator is that you can shift the harmonics' distribution on the frequency axis by the shape parameter. It is done by shortening the time of playing a single waveform while keeping the interval between each single waveform playback starts. The shorter the time, the higher the frequencies of the harmonics. You may compare it to the pulse-width modulation of pulse wave oscillators.

Moreover, Waves2 plays a single waveform as many as possible in one wavelength. It makes the sound much brighter than playing the waveform only once. To let the sound keeping its original pitch, Waves2 decreases little by little the amplitude of each waveform playbacks in the same cycle.

![Shape](https://raw.githubusercontent.com/boochow/Waves2/images/waves2.gif)

Some waveforms produce too much amount of harmonics when it comes to Waves2. You can make the harmonics distribution more gently by decreasing the amount of amplitude of repeated waveform playback with the shift-shape parameter.

![Shape](https://raw.githubusercontent.com/boochow/Waves2/images/waves2-alt.gif)

## Parameters

- The shape parameter shortens the actual playing time of a single waveform.

- The shift shape parameter decreases the amplitude of the repeated waves.

- "wave" is to select one from pre-defined 89 waveforms.

- "ShapeMod Intensity" adjusts the intensity of the inner envelope to the shape parameter.

- "Mod Attack" is the attack time of the inner envelope.

- "Mod Decay" is the decay time of the inner envelope.

- "ShapeLFO Attenuation" decreases the intensity of the LFO.

- "ModToAmp Intensity" increases the intensity of the inner envelope to the amplitude of the sound.


