#include <uClock.h>
#include <MIDI.h>

#define ERASE_PIN 2

MIDI_CREATE_DEFAULT_INSTANCE();

#define CHANNEL_COUNT 8
#define SEQUENCE_LENGTH 32

bool midiThru = false;

byte currentPosition = 0;
byte sequence[CHANNEL_COUNT][SEQUENCE_LENGTH];

byte previousNote[CHANNEL_COUNT];

void clockOutput16PPQN(uint32_t* tick) {

  for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      if (previousNote[channel] > 0) {
        MIDI.sendNoteOn(previousNote[channel], 0, channel);
        previousNote[channel] = 0;
      }

      byte currentNote = sequence[channel][currentPosition];

      if (currentNote > 0) {
        MIDI.sendNoteOn(currentNote, 127, channel);
        previousNote[channel] = currentNote;
      }
  }

  currentPosition = (currentPosition + 1) % SEQUENCE_LENGTH;
}

void setup() {
  //Serial.begin(31250);
  TCCR1B = TCCR1B & B11111000 | B00000001;  
  uClock.init();

  uClock.setClock16PPQNOutput(clockOutput16PPQN);

  uClock.setTempo(96);
  uClock.start();

  for (size_t i = 0; i < SEQUENCE_LENGTH; i++) {
    previousNote[i] = 0;
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      sequence[channel][i] = 0;
    }
  }

  pinMode(ERASE_PIN, INPUT);

  

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  MIDI.turnThruOff();
}

void loop() {
  byte erase = digitalRead(ERASE_PIN);
  if (erase > 0) {
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      sequence[channel][currentPosition] = 0;
    }
  }
  MIDI.read();
}

void handleNoteOn(byte channel, byte note, byte velocity){
  if (midiThru) {
    MIDI.sendNoteOn(note, velocity, channel);
  }

  sequence[channel][currentPosition] = note;
}

void handleNoteOff(byte channel, byte note, byte velocity){
  if (midiThru) {
    MIDI.sendNoteOff(note, velocity, channel);
  }
}
