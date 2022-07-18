#ifndef TRACK_H
#define TRACK_H
#include "MIDI.h"

#define ON 127
#define OFF 0

#define BASE_DRUM 36
#define BASE_DRUM_ERASE 84


#define DRUM_COUNT 7 

MIDI_CREATE_DEFAULT_INSTANCE();

byte whiteKeys[DRUM_COUNT] = {0,2,4,5,7,9,11};

class Track {
public:
  Track(byte channel) : channel(channel) {
    for (size_t i = 0; i < SEQUENCE_LENGTH_MAX; i++) {
      sequence[i] = 0;
    }
  }
  
public:
  byte transpose = 0;
  byte sequence[SEQUENCE_LENGTH_MAX];
  byte previousNote = 0;

public:
  bool drumMode = false;
  bool isMuted = false;
  byte channel;

public:

  void triggerNoteOn(byte note, bool enableTranspose) {

    if (note > 0 && !isMuted) {
      if (drumMode) {
        playDrum(note, true);
      } else {
        if (enableTranspose) {
          note += transpose;
        }
        MIDI.sendNoteOn(note, ON, channel + 1);
      }
      previousNote = note;
    }
  }

  void triggerNoteOff() {
    if (previousNote > 0) {
      if (drumMode) {
        playDrum(previousNote, false);
      } else {
        MIDI.sendNoteOn(previousNote, OFF, channel + 1);
      }
      previousNote = 0;
    }
  }

  void playDrum(byte note, bool isOn) {
    for (byte i = 0; i < DRUM_COUNT; i++) {
      if ((note & (1<<i)) == (1<<i)) {
        MIDI.sendNoteOn(whiteKeys[i] + BASE_DRUM, isOn ? ON : OFF, channel + 1);
      }
    }
  }

  void removeDrum(byte note) {
    byte dmNote = note % 12;
    for (byte i = 0; i < DRUM_COUNT; i++) {
      if (whiteKeys[i] == dmNote) {
        for (int step = 0; step < SEQUENCE_LENGTH_MAX; step++) {
          sequence[step] &= ~(1<<i);
        }
        break;
      }
    }
  }

  void recNote(byte step, byte note) {
    if (drumMode) {
      byte dmNote = note % 12;
      for (byte i = 0; i < DRUM_COUNT; i++) {
        if (whiteKeys[i] == dmNote) {
          sequence[step] |= (1<<i);
          break;
        }
      }
    } else {
      sequence[step] = note;
    }
  }
  
  void switchDrumMode() {
    setDrumMode(!drumMode);
  }

  void setDrumMode(bool state) {
    eraseAll();
    drumMode = state;
  }

  void eraseAll() {
    for (int step = 0; step < SEQUENCE_LENGTH_MAX; step++) {
      sequence[step] = 0;
    }
  }
};


#endif //TRACK_H
