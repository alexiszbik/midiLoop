#include <uClock.h>
#include "MIDI.h"

#include "ArpState.h"
#include "Switch.h"
#include "Sequencer.h"

//Note -> 
//mute channel 1 = 60
//mute channel 2 = 61
//mute channel 3 = 62
//mute channel 4 = 63
//mute gate generator = 64

#define TEMPO_ANALOG_IN 0
#define BARCOUNT_ANALOG_IN 1
#define STEPCOUNT_ANALOG_IN 2
#define MIDITHRU_ANALOG_IN 3
#define TRANSPOSE_ANALOG_IN 4
#define SHIFT_ANALOG_IN 5
#define ERASE_ANALOG_IN 6
#define PLAYSTOP_ANALOG_IN 7

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

#define BEATOUT_LED 12

#define TEMPO_MIN 30
#define TEMPO_MAX 200

#define LOOPER_CHANNEL 12
#define LED_STRIPE_CHANNEL 13

#define MUTE_GATE_NOTE 64

//THIS is for my personal use
//This will map channel 5,6,7,8 as channel 1,2,3,4 MIDI thru
#define USE_MIDI_THRU_CHANNELS 1

#define BASE_NOTE 60;

byte ledPins[CHANNEL_COUNT] = {CHANNEL_1_LED, CHANNEL_2_LED, CHANNEL_3_LED, CHANNEL_4_LED};

Switch channelSwitches[CHANNEL_COUNT] = {Switch(CHANNEL_1_PIN), Switch(CHANNEL_2_PIN), Switch(CHANNEL_3_PIN), Switch(CHANNEL_4_PIN)};
Switch playStopSwitch(PLAYSTOP_ANALOG_IN, true);

bool midiThruChannels = (bool)USE_MIDI_THRU_CHANNELS;

bool shiftIsPressed = false;

unsigned long delayStart = 0;
bool delayIsRunning = false;

uint32_t midiTick = 0;
bool useMidiClock = false;

bool isPlaying = false;

bool needsToSendMidiStart = false;

bool midiThru = false;
bool transposeMode = false;
byte currentChannel = 0;

byte currentPosition = 0;

ArpState arpState;
Sequencer seq;

bool arpIsOn = false;

bool gateIsMuted = false;

void clockOutput16PPQN(uint32_t* tick) {

  if (!isPlaying) {
    return;
  }

  bool isQuarterBeat = (((currentPosition % seq.currentStepCount) % 4) == 0);
  digitalWrite(BEATOUT_LED, isQuarterBeat); 
    
  for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {

    seq[channel].triggerNoteOff();

    if (arpIsOn && currentChannel == channel && arpState.count > 0) {
      if (seq[channel].drumMode) {

        byte dmNote = seq[channel].sequence[currentPosition];
        
        for (byte i = 0; i < arpState.count; i++) {
          byte note = arpState.list[i];
          note = note % 12;
          for (byte i = 0; i < DRUM_COUNT; i++) {
            if (whiteKeys[i] == note) {
              dmNote |= (1<<i);
              break;
            }
          }
          seq[channel].triggerNoteOn(dmNote, false);

          if (!midiThru) {
            seq[currentChannel].recNote(currentPosition, note);
          }
        }
      } else {
        byte note = arpState.getNote();

        seq[channel].triggerNoteOn(note, false);
        displayIntValue(arpState.arpPos + 1);
  
        if (!midiThru) {
            seq[currentChannel].recNote(currentPosition, note);
        }
      }
    } else {
      byte currentNote = seq[channel].sequence[currentPosition];
      seq[channel].triggerNoteOn(currentNote, true);
    }
  }
    
  currentPosition = (currentPosition + 1) % seq.currentSeqLength();
}

void clockOutput32PPQN(uint32_t* tick) {

  if (!isPlaying) {
    return;
  }

  if (!gateIsMuted) {
    digitalWrite(OUT_GATE, (*tick % 2) == 0 ? HIGH : LOW);
  }
  
  digitalWrite(OUT_SYNC, (*tick % 4) < 2 ? HIGH : LOW);
}

void clockOutput96PPQN(uint32_t* tick) {
  if (needsToSendMidiStart) {
    needsToSendMidiStart = false;
    Serial.write(0xFA);
  }
  Serial.write(0xF8);
}

void setup() {
  //Serial.begin(31250);
  TCCR1B = TCCR1B & B11111000 | B00000001;
  uClock.init();

  uClock.setClock16PPQNOutput(clockOutput16PPQN);
  uClock.setClock32PPQNOutput(clockOutput32PPQN);
  uClock.setClock96PPQNOutput(clockOutput96PPQN);

  uClock.setTempo(96);
  uClock.start();

  pinMode(CHANNEL_1_LED, OUTPUT);
  pinMode(CHANNEL_2_LED, OUTPUT);
  pinMode(CHANNEL_3_LED, OUTPUT);
  pinMode(CHANNEL_4_LED, OUTPUT);

  pinMode(CHANNEL_1_PIN, INPUT);
  pinMode(CHANNEL_2_PIN, INPUT);
  pinMode(CHANNEL_3_PIN, INPUT);
  pinMode(CHANNEL_4_PIN, INPUT);

  pinMode(BEATOUT_LED, OUTPUT);

  pinMode(OUT_GATE, OUTPUT);
  pinMode(OUT_SYNC, OUTPUT);

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);

  MIDI.setHandleStart(handleStart);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleClock(handleClock);

  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandleControlChange(handleControlChange);
  
  MIDI.begin(MIDI_CHANNEL_OMNI);

  MIDI.turnThruOff();
}

void handleShift() {
  int shiftState = analogRead(SHIFT_ANALOG_IN);
  shiftIsPressed = (shiftState > 200);
}

void handleErase() {
  int erase = analogRead(ERASE_ANALOG_IN);

  if (erase > 200) {
    if (!shiftIsPressed) {
      seq[currentChannel].sequence[currentPosition] = 0;
    } else {
      seq[currentChannel].eraseAll();
    }
  }
}

void handleTempo() {
  int tempoPot = analogRead(TEMPO_ANALOG_IN);
  float tempo = round(((float)tempoPot/1024.f)*(TEMPO_MAX - TEMPO_MIN) + TEMPO_MIN);

  uClock.setTempo(tempo);
}

void handleBarCount() {
  int pot = analogRead(BARCOUNT_ANALOG_IN);
  byte maxValue = SEQUENCE_LENGTH_MAX/STEP_PER_BAR_MAX;
  int nextBarCount = round(((float)pot/1024.f)*(maxValue - 1) + 1);
  
  if (nextBarCount != seq.currentBarCount) {
    seq.currentBarCount = nextBarCount;
    displayIntValue(nextBarCount);
  }
}

void handleStepCount() {
  int pot = analogRead(STEPCOUNT_ANALOG_IN);
  int nextStepCount = round(((float)pot/1024.f)*(STEP_PER_BAR_MAX - 1) + 1);

  if (nextStepCount != seq.currentStepCount) {
    seq.currentStepCount = nextStepCount;
    displayIntValue(nextStepCount);
  }
}

void displayIntValue(int value) {
  int b[4] = {1,2,4,8};

   for (int i = 0; i < CHANNEL_COUNT; i++) {
     int state = value & b[i];
     digitalWrite(ledPins[i], (state > 0) ? HIGH : LOW);
   }

   delayStart = millis();
   delayIsRunning = true;
}


void handleMidiThru() {
  bool midiThruState = analogRead(MIDITHRU_ANALOG_IN) > 200;
 
  if (!midiThruState && midiThru) {
    //do some midi panic
  }

  midiThru = midiThruState;
}

void handleTranspose() {
  int transposeState = analogRead(TRANSPOSE_ANALOG_IN);
  transposeMode = (transposeState > 200);
}

void handleCurrentChannel() {
  for (byte i = 0; i < CHANNEL_COUNT; i++) {
    if (channelSwitches[i].debounce()) {
      bool state = channelSwitches[i].getState();
      if (state) {
        if (shiftIsPressed) {
          switch(i) {
            case 0 : seq.fill();
            break;
            case 1 : arpIsOn = !arpIsOn;
            break;
            case 3 : seq[currentChannel].switchDrumMode();
            break;
            default : break;
          }
        } else {
          currentChannel = i;
        }
      }
    }
  }
  if (!delayIsRunning) {
    for (byte i = 0; i < CHANNEL_COUNT; i++) {
      bool state = i == (currentChannel); 
      digitalWrite(ledPins[i], state ? HIGH : LOW);
    }
  }
}

void setIsPlaying(bool state) {
  isPlaying = state;
  
  if (state) {
    uClock.stop();

    needsToSendMidiStart = true;

    if (!useMidiClock) {
      uClock.start();
    }
    
  } else {
    Serial.write(0xFC);

    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      if (seq[channel].previousNote > 0) {
        MIDI.sendNoteOn(seq[channel].previousNote, 0, channel + 1);
        seq[channel].previousNote = 0;
      }
    }
        
    digitalWrite(OUT_GATE, LOW);
    digitalWrite(OUT_SYNC, LOW);
    digitalWrite(BEATOUT_LED, LOW); 

    currentPosition = 0;
        
  }
}

void handleStartStop() {
  if (playStopSwitch.debounce()) {
    if (playStopSwitch.getState()) {
      if (shiftIsPressed) {
        currentPosition = 0;
      } else {
        setIsPlaying(!isPlaying);
      }
    }
  }
}

byte actionCounter = 0;

void loop() {

  actionCounter++;

  if (actionCounter > 100) {

    handleShift();
    
    handleTempo();
    handleBarCount();
    handleStepCount();
    
    handleMidiThru();
    handleTranspose();
    handleStartStop();
    handleCurrentChannel();
    
    handleErase();

    if ((millis() - delayStart) >= 2000 && delayIsRunning) {
      delayIsRunning = false;
    }
    actionCounter = 0;
  }
  
  MIDI.read();
}

void handleNoteOn(byte channel, byte note, byte velocity) {

  if (channel == LED_STRIPE_CHANNEL) {
    
  } else if (midiThruChannels && (channel >= 5 && channel <= 8) && isPlaying) {
    MIDI.sendNoteOn(note, velocity, channel - 4);
    
  } else if (channel == LOOPER_CHANNEL && note >= 60 && note <= 63) {
    byte channel = note - 60;
    seq[channel].isMuted = (velocity > 0);

  } else if (channel == LOOPER_CHANNEL && note == MUTE_GATE_NOTE) {
    gateIsMuted = (velocity > 0);
  } else if (channel == LOOPER_CHANNEL && note == 70) {
    if (velocity > 0) {
      seq.eraseAll();
    }
  } else if (seq[currentChannel].drumMode && note >= BASE_DRUM_ERASE) {
      seq[currentChannel].removeDrum(note);
  } else {
    arpState.addNote(note);
    
    if ((midiThru && !arpIsOn) || !isPlaying) {
      MIDI.sendNoteOn(note, velocity, channel);
    } else {
      if (transposeMode) {
        seq[currentChannel].transpose = note - BASE_NOTE;
        
      } else {
        if (!arpIsOn) {
          seq[currentChannel].recNote(currentPosition, note);
        }
      }
    }
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (channel == LED_STRIPE_CHANNEL) {
    
  } else if (midiThruChannels && (channel >= 5 && channel <= 8) && isPlaying) {
    MIDI.sendNoteOff(note, velocity, channel - 4);
    
  } else if (channel == LOOPER_CHANNEL && note >= 60 && note <= 63) {
    byte channel = note - 60;
    seq[channel].isMuted = false;
    
  } else if (channel == LOOPER_CHANNEL && note == MUTE_GATE_NOTE) {
    gateIsMuted = false;
  } else {
      arpState.removeNote(note);
      if (arpState.count == 0) {
        delayIsRunning = false;
      }
      
      if ((midiThru && !arpIsOn) || !isPlaying) {
        MIDI.sendNoteOff(note, velocity, channel);
      }
  }
}

void handlePitchBend(byte channel, int bend) {

  if (midiThru) {
    MIDI.sendPitchBend(bend, channel);
  }
}

void handleProgramChange(byte channel, byte number) {

  if (midiThruChannels && (channel >= 5 && channel <= 8)) {
    MIDI.sendProgramChange(number, channel - 4);
  } else {
    MIDI.sendProgramChange(number, channel);
  }
}

void handleControlChange(byte channel, byte control, byte value) {

  if (midiThruChannels && (channel >= 5 && channel <= 8)) {
    MIDI.sendControlChange(control, value, channel - 4);
  } else {
    MIDI.sendControlChange(control, value, channel);
  }
}

void handleStart() {
  useMidiClock = true;
  setIsPlaying(true);
  midiTick = 0;
}

void handleStop() {
  setIsPlaying(false);
  useMidiClock = false;
}

void handleClock() {
 
  if (midiTick % 6 == 0) {
    uint32_t _step = midiTick / 6;
    clockOutput16PPQN(&_step);
  }

  if (midiTick % 3 == 0) {
    uint32_t half = midiTick / 3;
    clockOutput32PPQN(&half);
  }
  midiTick = midiTick + 1;
  
}
