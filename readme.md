dmlitefix
=========

by Jari Komppa 2018, http://iki.fi/sol

This is a little hack to make it possible to use the Alesis DM Lite drum kit with
some VSTs. It's not a perfect thing, and I may continue developing it depending
on feedback. Don't hold your breath though.

The problem is that the DM Lite kit sends MIDI commands as "99 xx yy" whereas
the VSTs seem to expect "90 xx yy", plus the note values are not some that
drum VSTs like. I guess?

To use this "fix" you need a MIDI loopback device such as loopmidi. First run the
loopback device, create a loopback port. Then run the fix, selecting the DM Lite
kit as input and loopback as output. Finally run your DAW and select loopback
device as input. If everything went fine, something should now work. Maybe.

I've hardcoded the note values to what NI Polyplex expects, which is probably
not what you want. Sorry about that. I could make this configurable, but haven't
so far. Depends on the feedback and all that, you know.

Mapping goes:

- 24 -> 3c (foot pedal)
- 2e -> 3e (left dish)
- 31 -> 40 (middle dish)
- 33 -> 41 (right dish)
- 26 -> 43 (leftmost pad)
- 30 -> 45 (next pad)
- 2d -> 47 (..)
- 2b -> 48 (rightmost pad)

To build, you need the rtmidi library. No other dependencies. I only provide windows
binaries, but it should(tm) compile on other platforms too. Maybe. _getch() may be a
problem, as it's DOS conio stuff.

Use at your own risk, but other than that, feel free to use in any way you want.
