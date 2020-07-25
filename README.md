# Waves2 Oscillator

This is a custom user oscillator for minilogue xd and NTS-1.
You can download binaries from [the release page](https://github.com/boochow/waves2/releases).

## About

Waves2 is the oscillator which can control its harmonics WITHOUT filters.It is a wavetable oscillator that uses pre-defined waveforms in the logue SDK, the same as the pre-installed custom oscillator, "Waves." The main characteristic of this oscillator is that you can shift up harmonics' frequency while keeping its structure. It is done by shortening the time of playing a single waveform while keeping the interval of the start timing of generating a single waveform. The shorter the single playback term, the higher the frequency of the harmonics. 

Although it is similar to pulse-width modulation of the pulse wave oscillator, Waves2 plays a single waveform multiple times in one cycle. It makes the sound much brighter than playing the waveform only once.

![Shape](https://raw.githubusercontent.com/boochow/Waves2/images/waves2.gif)

While playing the waveform multiple times in a single cycle may seem to be the same as the pitch shifting, the amplitude of the repeated waveform is getting smaller than the first playback, so the sound is different from mere pitch shifting.

Some waveforms produce too much harmonics amount when it comes to Waves2. You can decrease the amplitude of repeated waveform playback. It results in reducing the amount of higher harmonics.

![Shape](https://raw.githubusercontent.com/boochow/Waves2/images/waves2-alt.gif)

## Parameters

- The shape parameter increases the number of times the waveform is repeated in one cycle.

- The shift shape parameter decreases the amplitude of the repeated waves.

- The "wave" parameter is to select one from pre-defined 89 waveforms.