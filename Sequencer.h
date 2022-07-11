#ifndef SEQUENCER_H
#define SEQUENCER_H

#define CHANNEL_COUNT 4
#define SEQUENCE_LENGTH_MAX 128
#define STEP_PER_BAR_MAX 16

class Track {
public:
   byte transpose = 0;
   byte sequence[SEQUENCE_LENGTH_MAX];
   byte previousNote = 0;
};

class Sequencer {
  
  Track tracks[CHANNEL_COUNT] = {Track(), Track(), Track(), Track()};
public:
  byte currentBarCount = 1;
  byte currentStepCount = STEP_PER_BAR_MAX;

  Track& operator [] (int i) {return tracks[i];}

  int currentSeqLength() {
    return currentBarCount * currentStepCount;
  }

  void eraseAll() {
    for (byte channel = 0; channel < CHANNEL_COUNT; channel++) { 
      for (size_t step = 0; step < SEQUENCE_LENGTH_MAX; step++) {
        tracks[channel].sequence[step] = 0;
      }
    }
  }

  void fill() {
    byte seqLen = currentSeqLength();
    for (int i = seqLen; i < SEQUENCE_LENGTH_MAX; i++) {
      for (byte channel = 0; channel < CHANNEL_COUNT; channel++) {
        tracks[channel].sequence[i] = tracks[channel].sequence[i % seqLen];
      }
    }
  }
};

#endif
