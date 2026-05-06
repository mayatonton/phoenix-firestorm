# AYAstorm r11 — Bundled Venue IRs

This directory holds the impulse-response (IR) WAV files used by the
AYAstorm Stream3D venue convolution reverb (`LLVenueReverbDsp`, r11 P7c).

The viewer attempts to load `<name>.wav` for each non-`dry` venue in the
catalog at startup. Missing or rejected files leave the slot un-selectable
(silent fallback to dry). See `doc/spec_binaural_venue_reverb.md` §4.4.5
for the canonical bundling rules.

## File requirements

Each file must satisfy **all** of the following or the slot is dropped:

- Container: RIFF / WAVE
- Encoding: PCM 16-bit **or** IEEE float 32-bit
- Channels: 1 (mono — duplicated to L/R) or 2 (true stereo IR)
- Sample rate: must match the FMOD mixer sample rate (typically 48000 Hz
  on modern OS defaults, 44100 Hz fallback). No resampling is performed.
- Length: ≤ 3 s recommended; longer IRs work but cost CPU per partition.

## Catalog

| File              | Venue name      | Source / License | Attribution |
|-------------------|-----------------|------------------|-------------|
| `room_small.wav`  | `room_small`    | _TBD_            | _TBD_       |
| `room_medium.wav` | `room_medium`   | _TBD_            | _TBD_       |
| `hall_small.wav`  | `hall_small`    | _TBD_            | _TBD_       |
| `hall_medium.wav` | `hall_medium`   | _TBD_            | _TBD_       |
| `hall_large.wav`  | `hall_large`    | _TBD_            | _TBD_       |
| `club.wav`        | `club`          | _TBD_            | _TBD_       |
| `cathedral.wav`   | `cathedral`     | _TBD_            | _TBD_       |
| `outdoor.wav`     | `outdoor`       | _TBD_            | _TBD_       |

`dry` is not file-backed — selecting it bypasses the reverb DSP entirely.

## Acceptable IR sources

Only IRs with the following licenses may be bundled (per spec §4.4.5):

- **CC0 / Public Domain** — preferred, no attribution required
- **CC-BY 3.0 / CC-BY 4.0** — attribution must be added to the table above
- **Apache 2.0** — attribution must be added to the table above

Recommended public IR libraries:

- [OpenAIR](https://www.openair.hosted.york.ac.uk/) — mostly CC-BY 4.0
- [EchoThief](http://www.echothief.com/) — CC-BY 4.0
- [MIT IR Survey](https://mcdermottlab.mit.edu/Reverb/IR_Survey.html) — CC-BY 3.0

When a new file is added, fill in the row above with the source URL and the
exact license name, then commit `CREDITS.md` together with the WAV.
