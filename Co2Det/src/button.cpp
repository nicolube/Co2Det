#include <button.h>
#include <Arduino.h>

void Button::tick() {
    unsigned long m = millis();
    if (digitalRead(this->input)) {
        this->lMillis = m; isTriggered = false;
        }
    unsigned short d = m - lMillis;
    if (d > 200 && !isTriggered) {
        isTriggered = true;
        trigger();
    } 
}

void Button::init(Trigger trigger) {
    this->trigger = trigger;
    pinMode(input, INPUT_PULLUP);
}