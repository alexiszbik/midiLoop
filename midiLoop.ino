#include <uClock.h>
#include <MIDI.h>

#define TEMPO_ANALOG_IN 0
#define MIDITHRU_ANALOG_IN 3
#define TRANSPOSE_ANALOG_IN 4
#define PLAYSTOP_ANALOG_IN 5
#define ERASE_ANALOG_IN 6

#define CHANNEL_4_LED 2
#define CHANNEL_3_LED 3
#define CHANNEL_1_LED 4 // Yes, I inverted the 1 & 2 LEDS when I built my box :|
#define CHANNEL_2_LED 5

#define OUT_GATE 6
#define OUT_SYNC 7

#define CHANNEL_4_PIN 8
#define CHANNEL_3_PIN 9
#define CHANNEL_2_PIN 10
#define CHANNEL_1_PIN 11

#define BEATOUT_PIN 12

#define CHANNEL_COUNT 5
#define SEQUENCE_LENGTH 32

MIDI_CREATE_DEFAULT_INSTANCE();

bool currentPlayStopSwitchState = false;

bool midiThru = false;
bool transposeMode = false;
byte currentChannel = 1;

byte transpose[CHANNEL_COUNT];
byte baseNote = 60;

byte currentPosition = 0;
byte sequence[CHANNEL_COUNT][SEQUENCE_LENGTH];

byte previousNote[CHANNEL_COUNT];

void clockOutput16PPQN(uint32_t* tick) {

  bool isQuarterBeat = ((currentPosition % 4) == 0);
  digitalWrite(BEATOUT_PIN, isQuarterBeat); 

  for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      if (previousNote[channel] > 0) {
          MIDI.sendNoteOn(previousNote[channel], 0, channel);
          previousNote[channel] = 0;
      }

      byte currentNote = sequence[channel][currentPosition];

      if (currentNote > 0) {
          currentNote += transpose[channel];
        
          MIDI.sendNoteOn(currentNote, 127, channel);
          previousNote[channel] = currentNote;
      }
  }

  currentPosition = (currentPosition + 1) % SEQUENCE_LENGTH;
}

void clockOutput32PPQN(uint32_t* tick) {
  digitalWrite(OUT_GATE, (*tick % 2) == 0);
  digitalWrite(OUT_SYNC, (*tick % 4) < 2);
}

void setup() {
  //Serial.begin(31250);
  TCCR1B = TCCR1B & B11111000 | B00000001;  
  uClock.init();

  uClock.setClock16PPQNOutput(clockOutput16PPQN);
  uClock.setClock32PPQNOutput(clockOutput32PPQN);

  uClock.setTempo(96);
  uClock.start();

  for (size_t i = 0; i < SEQUENCE_LENGTH; i++) {
    previousNote[i] = 0;
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      sequence[channel][i] = 0;
    }
  }

  for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      transpose[channel] = 0;
  }

  pinMode(CHANNEL_1_LED, OUTPUT);
  pinMode(CHANNEL_2_LED, OUTPUT);
  pinMode(CHANNEL_3_LED, OUTPUT);
  pinMode(CHANNEL_4_LED, OUTPUT);

  pinMode(CHANNEL_1_PIN, INPUT);
  pinMode(CHANNEL_2_PIN, INPUT);
  pinMode(CHANNEL_3_PIN, INPUT);
  pinMode(CHANNEL_4_PIN, INPUT);

  pinMode(BEATOUT_PIN, OUTPUT);

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  MIDI.turnThruOff();
}

void handleTempo() {
  int tempoPot = analogRead(0);
  float tempo = ((float)tempoPot/1024.f)*210.f + 30.f;

  uClock.setTempo(tempo);
}

void handleMidiThru() {
  int midiThruState = analogRead(MIDITHRU_ANALOG_IN);
  midiThru = (midiThruState > 200);
}

void handleTranspose() {
  int transposeState = analogRead(TRANSPOSE_ANALOG_IN);
  transposeMode = (transposeState > 200);
}

void handleCurrentChannel() {
  bool channelStates[4] = {false, false, false, false};
  
  channelStates[0] = digitalRead(CHANNEL_1_PIN) == HIGH;
  channelStates[1] = digitalRead(CHANNEL_2_PIN) == HIGH;
  channelStates[2] = digitalRead(CHANNEL_3_PIN) == HIGH;
  channelStates[3] = digitalRead(CHANNEL_4_PIN) == HIGH;

  for (byte i = 0; i < 4; i++) {
    if (channelStates[i]) {
      currentChannel = i+1;
    }
  }

  byte ledPins[4] = {CHANNEL_1_LED, CHANNEL_2_LED, CHANNEL_3_LED, CHANNEL_4_LED};

  for (byte i = 0; i < 4; i++) {
    bool state = i == (currentChannel-1); 
    digitalWrite(ledPins[i], state ? HIGH : LOW);
  }
  
}

void toggleStartStop() {
  
}

void handleStartStop() {
  int switchState = analogRead(PLAYSTOP_ANALOG_IN);

  if ((switchState == HIGH) && !currentPlayStopSwitchState) {
    toggleStartStop();
    currentPlayStopSwitchState = true;
  }
  
  if (switchState == LOW) {
    currentPlayStopSwitchState = false;
  }
}

void loop() {

  handleTempo();
  handleMidiThru();
  handleTranspose();
  handleStartStop();
  handleCurrentChannel();
  
  int erase = analogRead(ERASE_ANALOG_IN);

  if (erase > 200) {
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      sequence[currentChannel][currentPosition] = 0;
    }
  }
  
  MIDI.read();
}

void handleNoteOn(byte channel, byte note, byte velocity){
  if (midiThru) {
    MIDI.sendNoteOn(note, velocity, channel);
  } else {
    if (transposeMode) {
      transpose[currentChannel] = note - baseNote;
    } else {
      sequence[currentChannel][currentPosition] = note;
    }
  }
}

void handleNoteOff(byte channel, byte note, byte velocity){
  if (midiThru) {
    MIDI.sendNoteOff(note, velocity, channel);
  }
}
