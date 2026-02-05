# midiLoop
A nice MIDI looper/sequencer made with Arduino

This project use uClock by midilab : https://github.com/midilab/uClock
Use the version uClock 1.5 ! Otherwise the code won't compile.

Up to date circuit :

![MidiLoop schematic](schematic.png)

---

## Features

- Up to **128 steps** total
- **4 independent channels**
- Adjustable **bar count (1–8)** and **steps per bar (1–16)**
- Real‑time recording with automatic step quantization
- MIDI **IN / OUT / THRU**
- MIDI **START / STOP / CLOCK** send & receive
- CV **SYNC OUT** and **GATE OUT** (optional)
- Optional **arp** and **drum mode** per channel

---

This sequencer is intentionally simple. Some things are missing on purpose:

- Only **4 channels** (honestly: enough for any serious bleeps & bloops)
- **Monophonic** sequences — one note per step, per channel
- No note length — **1 step = 1 note** (use your synth’s ADSR)
- No velocity recording — everything plays at **velocity 127**

Limitations force creativity !

---

## Recording Behavior

- Notes are **quantized to the next step**
- What you hear is what lands on that next step
- It may feel strange at first, but it guarantees everything stays perfectly in sync

(Other quantization strategies *could* exist in the MidiLoop v2. Stay tuned !)

---

## Controls

### Channel Select (1–4) + LEDs
Select the active channel for recording or editing.

The LEDs show:
- The currently selected channel (normal mode)
- Binary values when adjusting bars or steps

---

### PLAY / STOP
- Starts or stops the sequencer
- **SHIFT + PLAY**: restart from step 1 ("cue" style)

---

### ERASE
- Erases notes *as they are played* on the current channel
- **SHIFT + ERASE**: clear the entire sequence of the current channel

(No global erase)

---

### TEMPO
- From 30 to 200 bpm (easy to tweak in code : change TEMPO_MIN & TEMPO_MAX values)
- Works with internal or external MIDI clock

---

### BAR COUNT & STEP COUNT

- **Bars**: 1–8
- **Steps per bar**: 1–16

---

### MIDI THRU
- When ON: incoming MIDI passes straight through
- Notes are *not* recorded
- Useful for live playing or solos

Note: MIDI channel stays that of your keyboard, not the sequencer.

---

### TRANSPOSE

- Enables transpose mode for the current channel
- Press a key to transpose the sequence
- **C4** = no transposition

---

### SHIFT

Extends multiple controls:

- **SHIFT + PLAY** → restart sequence
- **SHIFT + ERASE** → erase channel
- **SHIFT + CHANNEL 1** → fill entire sequence (pattern repeat)
- **SHIFT + CHANNEL 2** → toggle arpeggiator
- **SHIFT + CHANNEL 3** → arp panic
- **SHIFT + CHANNEL 4** → toggle drum mode

---

## MIDI I/O

### MIDI IN / OUT

You know the drill.

### MIDI Sync

- Sends and receives **START / STOP / CLOCK**
- Can act as master or slave

### MIDI Control Channel

A dedicated **Looper Control Channel** (default: channel 12, tweakable) allows remote control:

#### Notes
- Select channel: `50–53`
- Mute channel: `60–63` velocity 0 = OFF, velocity 127 = ON
- Mute gate: `64` velocity 0 = OFF, velocity 127 = ON
- Erase all: `70`

#### CCs
- Bar count: `CC 10` value = bar count
- Arp on/off: `CC 20` value 0 = OFF, value 127 = ON
- Fill sequence: `CC 30`

---

## CV Outputs

### SYNC OUT
- Clock signal while running
- Useful for syncing external gear

### GATE OUT
- Outputs a gate on every step
- Can be muted via MIDI
- Mainly designed for my personal DIY modular setup

---


## Hardware Notes

- I use Arduino Nano, not Nano Every. Uno works.
- Some LED pins may be inverted because… stuff happens

---

## Status

This is a personal project.

It works, and it does exactly what I want it to do — nothing more, nothing less.

If you build one, modify it, or break it in an interesting way: ✨ wonderful ✨

Send me an email with your version builded, I always like to see it !

---

## License

Do whatever you want. Attribution appreciated.
