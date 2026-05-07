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

The drop-in pipeline (`doc/r11/fetch_venue_irs.sh`'s `ir_convert` helper)
normalises every file to **48 kHz / 16-bit / mono / 3 s trim + 0.05 s
trailing fade**, which always satisfies the requirements above.

## Catalog

All bundled IRs are sourced from [OpenAIR](https://www.openair.hosted.york.ac.uk/)
under the **Creative Commons Attribution 4.0 International License**
(CC-BY 4.0). Attribution per IR below; the OpenAIR project itself is
credited at `www.openairlib.net`.

| File              | Venue name      | OpenAIR source | Attribution |
|-------------------|-----------------|----------------|-------------|
| `room_small.wav`  | `room_small`    | [Terry's Typing Room](https://www.openair.hosted.york.ac.uk/?page_id=740) | www.openairlib.net; Audiolab, University of York; Dr. Damian T. Murphy |
| `room_medium.wav` | `room_medium`   | [Spring Lane Building, University of York](https://www.openair.hosted.york.ac.uk/?page_id=670) | www.openairlib.net; Audiolab, University of York; Gavin Davies; Ignacio Gomez-Lanzaco; James Geary; Thomas Wood |
| `hall_small.wav`  | `hall_small`    | [The Dixon Studio Theatre, University of York](https://www.openair.hosted.york.ac.uk/?page_id=452) | www.openairlib.net; University of York; Ben Lavin; Darren Robinson; Ya-Hsin Chou |
| `hall_medium.wav` | `hall_medium`   | [St Andrew's Church](https://www.openair.hosted.york.ac.uk/?page_id=683) | www.openairlib.net; Audiolab, University of York; Damian T. Murphy |
| `hall_large.wav`  | `hall_large`    | [Usina del Arte Symphony Hall](https://www.openair.hosted.york.ac.uk/?page_id=770) | www.untref.edu.ar (Universidad Nacional de Tres de Febrero, Argentina) |
| `club.wav`        | `club`          | [Genesis 6 Studio Live Room (Drum Set Up)](https://www.openair.hosted.york.ac.uk/?page_id=483) | www.openairlib.net |
| `cathedral.wav`   | `cathedral`     | [York Minster](https://www.openair.hosted.york.ac.uk/?page_id=797) | www.openairlib.net; Audiolab, University of York; Damian T. Murphy |
| `outdoor.wav`     | `outdoor`       | [Koli National Park – Summer](https://www.openair.hosted.york.ac.uk/?page_id=577) | www.openairlib.net |

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

When swapping a row above, fill in the source URL and the exact attribution
required by that IR's ReadMe, then commit `CREDITS.md` together with the WAV.
