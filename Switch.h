#ifndef SWITCH_H
#define SWITCH_H

class Switch {
public:
  Switch(byte pin, bool analogPin = false) : pin(pin), analogPin(analogPin) {
  }

  bool debounce() {
    bool currentState = analogPin ? (analogRead(pin) > 200) : (digitalRead(pin) == HIGH);
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
  bool analogPin;

  bool state = false;
  bool stateChanged = false;
};


#endif //SWITCH_H
