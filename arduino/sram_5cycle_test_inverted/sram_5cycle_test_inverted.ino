// Address pins: A0-A4, A5-A9, A10-A14, A15-A17
const int ADDR[] = {2,3,4,5,6, 9,10,11,12,13, 32,33,34,35,36, 48,49,50};

// Data pins: IO0-IO7, IO8-IO15
const int DATA[] = {22,23,24,25,26,27,28,29, 37,38,39,40,41,42,43,44};

// Control pins
const int CE  = 7;
const int OE  = 47;
const int WE  = 8;
const int BLE = 45;
const int BHE = 46;

// Power switch pin -- INVERTED (HIGH = off, LOW = on)
const int PWR = 53;
const int PWR_ON  = LOW;
const int PWR_OFF = HIGH;

const int NUM_CYCLES = 10;
const uint32_t NUM_WORDS = 100;
const uint32_t OFF_DELAY_US = 3000000;   // 50 ms off
const uint32_t SETTLE_DELAY_US = 5000; // 5 ms to stabilize after power-on

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Power switch
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, PWR_OFF);

  // Control pins
  pinMode(CE,  OUTPUT); digitalWrite(CE,  HIGH);
  pinMode(OE,  OUTPUT); digitalWrite(OE,  HIGH);
  pinMode(WE,  OUTPUT); digitalWrite(WE,  HIGH);
  pinMode(BLE, OUTPUT); digitalWrite(BLE, LOW);
  pinMode(BHE, OUTPUT); digitalWrite(BHE, LOW);

  // Address pins
  for (int i = 0; i < 18; i++) pinMode(ADDR[i], OUTPUT);

  Serial.println("=== SRAM 5-CYCLE TEST ===");
  Serial.println();

  for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
    powerCycle();

    Serial.print("--- Cycle ");
    Serial.print(cycle + 1);
    Serial.println(" ---");

    for (uint32_t addr = 0; addr < NUM_WORDS; addr++) {
      uint16_t val = sramRead(addr);
      Serial.print("Word ");
      Serial.print(addr + 1);
      Serial.print(" = 0x");
      if (val < 0x1000) Serial.print("0");
      if (val < 0x0100) Serial.print("0");
      if (val < 0x0010) Serial.print("0");
      Serial.println(val, HEX);
    }
    Serial.println();
  }

  Serial.println("=== DONE ===");
}

void loop() {}

void powerCycle() {
  // Park control lines high before cutting power
  digitalWrite(CE, HIGH);
  digitalWrite(OE, HIGH);
  digitalWrite(WE, HIGH);

  digitalWrite(PWR, PWR_OFF);
  Serial.print("PWR set to OFF state, pin reads: ");
  Serial.println(digitalRead(PWR));
  delayMicroseconds(OFF_DELAY_US);

  digitalWrite(PWR, PWR_ON);
  Serial.print("PWR set to ON state, pin reads: ");
  Serial.println(digitalRead(PWR));
  delayMicroseconds(SETTLE_DELAY_US);
}

void setAddress(uint32_t addr) {
  for (int i = 0; i < 18; i++) {
    digitalWrite(ADDR[i], (addr >> i) & 1);
  }
}

uint16_t sramRead(uint32_t addr) {
  for (int i = 0; i < 16; i++) pinMode(DATA[i], INPUT);

  setAddress(addr);

  digitalWrite(CE, LOW);
  digitalWrite(OE, LOW);
  delayMicroseconds(5);

  uint16_t val = 0;
  for (int i = 0; i < 16; i++) {
    if (digitalRead(DATA[i])) val |= (1 << i);
  }

  digitalWrite(OE, HIGH);
  digitalWrite(CE, HIGH);

  return val;
}