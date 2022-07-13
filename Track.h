#ifndef TRACK_H
#define TRACK_H
#include "MIDI.h"

#define ON 127
#define OFF 0

#define BASE_DRUM 36

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

  void triggerNoteOn(byte step) {
    byte currentNote = sequence[step];

    if (currentNote > 0 && !isMuted) {
      if (drumMode) {
        for (byte i = 0; i < DRUM_COUNT; i++) {
          if ((currentNote & (1<<i)) == (1<<i)) {
            MIDI.sendNoteOn(whiteKeys[i] + BASE_DRUM, ON, channel + 1);
          }
        }
      } else {
        currentNote += transpose;
        
        MIDI.sendNoteOn(currentNote, ON, channel + 1);
      }
      previousNote = currentNote;
    }
  }

  void triggerNoteOff() {
    if (previousNote > 0) {
      if (drumMode) {
        for (byte i = 0; i < DRUM_COUNT; i++) {
          if ((previousNote & (1<<i)) == (1<<i)) {
            MIDI.sendNoteOn(whiteKeys[i] + BASE_DRUM, OFF, channel + 1);
          }
        }
      } else {
        MIDI.sendNoteOn(previousNote, OFF, channel + 1);
      }
      previousNote = 0;
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
