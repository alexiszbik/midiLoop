#ifndef ARPSTATE_H
#define ARPSTATE_H

#define MAX_KEY_PRESSED 10

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

#endif //ARPSTATE_H