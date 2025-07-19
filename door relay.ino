#include <HomeSpan.h>

#define RELAY_PIN   22
#define DOOR_PIN    17
#define TIMEOUT_MS  60000  // 1 минута

struct AutoRelay : Service::Switch {
  SpanCharacteristic *power;
  unsigned long activatedAt = 0;
  bool isOn = false;

  AutoRelay(int pin) : Service::Switch() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);  // выключено по умолчанию
    power = new Characteristic::On(false);
  }

  boolean update() override {
    if (power->getNewVal()) {
      digitalWrite(RELAY_PIN, HIGH);
      activatedAt = millis();
      isOn = true;
    } else {
      digitalWrite(RELAY_PIN, LOW);
      isOn = false;
    }
    return true;
  }

  void loop() override {
    if (isOn && millis() - activatedAt > TIMEOUT_MS) {
      digitalWrite(RELAY_PIN, LOW);
      power->setVal(false);
      isOn = false;
    }
  }
};

struct DoorSensor : Service::ContactSensor {
  SpanCharacteristic *state;
  int last = HIGH;
  unsigned long lastChange = 0;

  DoorSensor() : Service::ContactSensor() {
    pinMode(DOOR_PIN, INPUT_PULLUP);
    state = new Characteristic::ContactSensorState(1);  // 1 = закрыто
  }

  void loop() override {
    int current = digitalRead(DOOR_PIN);
    if (current != last && millis() - lastChange > 1000) {  // антидребезг 1с
      state->setVal(current == LOW ? 0 : 1);  // 0 = открыто
      last = current;
      lastChange = millis();
    }
  }
};

void setup() {
  Serial.begin(115200);
  homeSpan.setWifiCredentials("AirPort", "QWE");
  homeSpan.begin(Category::Bridges, "Loft Door");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Loft Relay");
    new AutoRelay(RELAY_PIN);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Door Sensor");
    new DoorSensor();
}

void loop() {
  homeSpan.poll();
}
