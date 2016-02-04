
# Use MidiFighter Twister to tweak parameter groups

## Mission Statement

This shall be a simple, drop in, utility to control parameters and float values using the MidiFighter Twister.

The emphasis is on ease of use.

## Goals: 

+ accept ofParameterGroup of up to 16 paramters
+ parameters may be either of type:

	1) float --> map to rotary control
	2) bool  --> map to switch control

+ current parameter state shall be shown on the midiFighter twister, and maintained.

+ unused parameters shall switch RGB LED of twister off

+ upper banks shall be used to fine-tune parameters, in deltas of 1/2 range max over 128, 1/4 range over 128, 1/16 range over 127, so each higher bank should give you double precision.

+ it should be self-contained


## Midi message structure:

Input: We're expecting our midi messges to arrive as CC messages.

		'CC' stands for continous controller.

		CC messages have the (most significant) first half-byte set to B,
		so anything xB... is a continous controller message.

		byte 0 .. controller message / channel number

		rotary controller values arrive on channel 0,
		switch controller values arrive on channel 1.

		The least significant, or the second half-byte is the channel
		on which the signal comes in.

		CC messages have two parameter bytes:

		byte 1 .. controller number
		byte 2 .. controller value

