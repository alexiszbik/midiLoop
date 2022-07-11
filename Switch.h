#ifndef SWITCH_H
#define SWITCH_H

class Switch {
public:
  Switch(byte pin) : pin(pin) {
  }

  bool debounce() {
    bool currentState = digitalRead(pin) == HIGH;
    if (currentState != state) {
      state = currentState;
      stateChanged = true;
    }
    
    return stateChanged;
  }

  bool getState() {
    stateChanged = false;
    return state;
  }

private:
  byte pin;

  bool state = false;
  bool stateChanged = false;
};


#endif //SWITCH_H
