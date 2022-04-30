#include <uClock.h>
#include <MIDI.h>

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

#define CHANNEL_COUNT 4
#define SEQUENCE_LENGTH_MAX 128
#define STEP_PER_BAR_MAX 16

#define TEMPO_MIN 30
#define TEMPO_MAX 200

#define MAX_KEY_PRESSED 10

#define LOOPER_CHANNEL 12
#define LED_STRIPE_CHANNEL 13

//THIS is for my personal use
//This will map channel 5,6,7,8 as channel 1,2,3,4 MIDI thru
#define USE_MIDI_THRU_CHANNELS 1

byte ledPins[4] = {CHANNEL_1_LED, CHANNEL_2_LED, CHANNEL_3_LED, CHANNEL_4_LED};

bool isMuted[4] = {false, false, false, false};

MIDI_CREATE_DEFAULT_INSTANCE();

bool midiThruChannels = (bool)USE_MIDI_THRU_CHANNELS;

bool shiftIsPressed = false;
bool fillIsDone = false;

unsigned long delayStart = 0;
bool delayIsRunning = false;

int currentBarCount = 1;
int currentStepCount = STEP_PER_BAR_MAX;

uint32_t midiTick = 0;
bool useMidiClock = false;

int currentSeqLength = 16;

bool isPlaying = false;
bool isPlayingSwitchState = false;

bool needsToSendMidiStart = false;

bool midiThru = false;
bool transposeMode = false;
byte currentChannel = 0;

byte transpose[CHANNEL_COUNT];
byte baseNote = 60;

byte currentPosition = 0;
byte sequence[CHANNEL_COUNT][SEQUENCE_LENGTH_MAX];

byte previousNote[CHANNEL_COUNT];

class ArpState {
  public:
  byte list[MAX_KEY_PRESSED];
  byte count = 0;
  byte arpPos = 0;

  void addNote(byte note) {
    if (count < MAX_KEY_PRESSED) {
      list[count] = note;
      count++;
    }
  }

  void removeNote(byte note) {
    bool pop = false;
    for (byte i = 0 ; i < count; i++) {
      if (list[i] == note) {
        pop = true;
      }
      if (pop && (i+1) < count) {
        list[i] = list[i+1];
      }
    }
    if (pop) {
      count--;
    }
  }

  byte getNote() {
    byte pos = arpPos;
    arpPos++;
    if (arpPos >= count) {
      arpPos = 0;
    }
    while (pos >= count && pos > 0) {
      pos--;
    }
    return list[pos];
  }
};

ArpState arpState;

bool arpIsOn = false;
bool arpPreviousState = false;

void clockOutput16PPQN(uint32_t* tick) {

  if (!isPlaying) {
    return;
  }

  bool isQuarterBeat = (((currentPosition % currentStepCount) % 4) == 0);
  digitalWrite(BEATOUT_LED, isQuarterBeat); 

  if (!arpIsOn) {
    
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      if (previousNote[channel] > 0) {
          MIDI.sendNoteOn(previousNote[channel], 0, channel + 1);
          previousNote[channel] = 0;
      }

      byte currentNote = sequence[channel][currentPosition];

      if (currentNote > 0 && !isMuted[channel]) {
          currentNote += transpose[channel];
        
          MIDI.sendNoteOn(currentNote, 127, channel + 1);
          previousNote[channel] = currentNote;
      }
    }
    
  } else {
    if (previousNote[currentChannel] > 0) {
       MIDI.sendNoteOn(previousNote[currentChannel], 0, currentChannel + 1);
      previousNote[currentChannel] = 0;
    }

    if (arpState.count > 0) {
      byte note = arpState.getNote();
      MIDI.sendNoteOn(note, 127, currentChannel + 1);
      previousNote[currentChannel] = note;
    }
  }

  currentPosition = (currentPosition + 1) % currentSeqLength;
}

void clockOutput32PPQN(uint32_t* tick) {

  if (!isPlaying) {
    return;
  }
  
  digitalWrite(OUT_GATE, (*tick % 2) == 0 ? HIGH : LOW);
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

  for (size_t i = 0; i < SEQUENCE_LENGTH_MAX; i++) {
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      sequence[channel][i] = 0;
    }
  }

  for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      transpose[channel] = 0;
      previousNote[channel] = 0;
  }

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
        sequence[currentChannel][currentPosition] = 0;
      } else {
        for (size_t step = 0; step < SEQUENCE_LENGTH_MAX; step++) {
          sequence[currentChannel][step] = 0;
        }
      }
  }
}

void eraseAll() {
  for (byte channel = 0; channel < CHANNEL_COUNT; channel++) { 
    for (size_t step = 0; step < SEQUENCE_LENGTH_MAX; step++) {
       sequence[channel][step] = 0;
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
  
  if (nextBarCount != currentBarCount) {
    currentBarCount = nextBarCount;
    currentSeqLength = currentBarCount * currentStepCount;

    displayIntValue(nextBarCount);
  }
}

void handleStepCount() {
  int pot = analogRead(STEPCOUNT_ANALOG_IN);
  int nextStepCount = round(((float)pot/1024.f)*(STEP_PER_BAR_MAX - 1) + 1);

  if (nextStepCount != currentStepCount) {
    currentStepCount = nextStepCount;
    currentSeqLength = currentBarCount * currentStepCount;

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

  if (shiftIsPressed) {
    
    if (channelStates[0] && !fillIsDone) { //FILL MODE
      fill();
    }
    if (channelStates[0]) {
      fillIsDone = false;
    }

    if (channelStates[1] && !arpPreviousState) {
      arpIsOn = !arpIsOn;
      arpPreviousState = true;
    }
    
  } else {
    for (byte i = 0; i < 4; i++) {
      if (channelStates[i]) {
        currentChannel = i;
      }
    }
  
    if (!delayIsRunning) {
      for (byte i = 0; i < CHANNEL_COUNT; i++) {
        bool state = i == (currentChannel); 
        digitalWrite(ledPins[i], state ? HIGH : LOW);
      }
    }
  }

  if (channelStates[1] == false) {
    arpPreviousState = false;
  }
}

void setIsPlaying(bool state) {
  isPlaying = state;
  
  if (state) {
    uClock.stop();
    //delay(20);
    needsToSendMidiStart = true;

    if (!useMidiClock) {
      uClock.start();
    }
    
  } else {
    Serial.write(0xFC);

    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      if (previousNote[channel] > 0) {
        MIDI.sendNoteOn(previousNote[channel], 0, channel + 1);
        previousNote[channel] = 0;
      }
    }
        
    digitalWrite(OUT_GATE, LOW);
    digitalWrite(OUT_SYNC, LOW);
    digitalWrite(BEATOUT_LED, LOW); 
    //delay(20);
    currentPosition = 0;
        
  }
}

void handleStartStop() {
  int startStopStart = analogRead(PLAYSTOP_ANALOG_IN);
  bool newPlayingState = (startStopStart > 200);

  if (newPlayingState != isPlayingSwitchState) {
    isPlayingSwitchState = newPlayingState;

    if (isPlayingSwitchState) {

      if (isPlaying && shiftIsPressed) {
        currentPosition = 0;
      } else {
        setIsPlaying(!isPlaying);
      }
    }
  }
}

void fill() {
  for (size_t i = currentSeqLength; i < SEQUENCE_LENGTH_MAX; i++) {
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
      sequence[channel][i] = sequence[channel][i % currentSeqLength];
    }
  }
  fillIsDone = true;
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
  
  
  //for debug purpose
  //digitalWrite(CHANNEL_1_LED, shiftIsPressed ? HIGH : LOW);
  
  MIDI.read();
}

void handleNoteOn(byte channel, byte note, byte velocity) {

  if (channel == LED_STRIPE_CHANNEL) {
    
  } else if (midiThruChannels && (channel >= 5 && channel <= 8)) {
    MIDI.sendNoteOn(note, velocity, channel - 4);
    
  } else if (channel == LOOPER_CHANNEL && note >= 60 && note <= 63) {
    byte channel = note - 60;
    isMuted[channel] = (velocity > 0);

  } else if (channel == LOOPER_CHANNEL && note == 70) {
    if (velocity > 0) {
      eraseAll();
    }
  } else {

    arpState.addNote(note);
  
    if (midiThru) {
      MIDI.sendNoteOn(note, velocity, channel);
    } else {
      if (transposeMode) {
        transpose[currentChannel] = note - baseNote;
        
      } else {
        
        if (arpIsOn) {
        
        } else {
          sequence[currentChannel][currentPosition] = note;
        }
      }
    }
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (channel == LED_STRIPE_CHANNEL) {
    
  } else if (midiThruChannels && (channel >= 5 && channel <= 8)) {
    MIDI.sendNoteOff(note, velocity, channel - 4);
    
  } else if (channel == LOOPER_CHANNEL && note >= 60 && note <= 63) {
    byte channel = note - 60;
    isMuted[channel] = false;
    
  } else {
      arpState.removeNote(note);
      
      if (midiThru) {
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
