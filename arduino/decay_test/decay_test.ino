// SRAM retention-decay test: write FILL, power off for OFF_DELAY, power on, read.
// Readback is compared against the gold reference (FILL) by decay_from_gold.py.
// Run once with FILL=0xFFFF (all-1s), once with FILL=0x0000 (all-0s).

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

const int NUM_CYCLES = 3;
const uint32_t NUM_WORDS = 1000;
const uint32_t OFF_DELAY_US = 50000;   // 1.75 s off (leakage decay is slow)
const uint32_t SETTLE_DELAY_US = 5000;   // 5 ms to stabilize after power-on

// Fill pattern written before each off-window.
// 0xFFFF = all-1s test, 0x0000 = all-0s test.
const uint16_t FILL = 0x0000;

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

  // Data pins -- declare as OUTPUT so we can force them LOW during the off-window
  for (int i = 0; i < 16; i++) pinMode(DATA[i], OUTPUT);

  Serial.print("=== SRAM DECAY TEST: ");
  Serial.print(NUM_CYCLES);
  Serial.print(" cycles, off_delay=");
  Serial.print(OFF_DELAY_US / 1000);
  Serial.print("ms, fill=0x");
  if (FILL < 0x1000) Serial.print("0");
  if (FILL < 0x0100) Serial.print("0");
  if (FILL < 0x0010) Serial.print("0");
  Serial.print(FILL, HEX);
  Serial.println(" ===");
  Serial.println();

  for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
    powerCycle();
    Serial.print("# cycle ");
    Serial.println(cycle);
    for (uint32_t addr = 0; addr < NUM_WORDS; addr++) {
      uint16_t val = sramRead(addr);
      if (val < 0x1000) Serial.print("0");
      if (val < 0x0100) Serial.print("0");
      if (val < 0x0010) Serial.print("0");
      Serial.print(val, HEX);
      if (addr < NUM_WORDS - 1) Serial.print(",");   // comma between words, not after the last
    }
    Serial.println();   // newline ends the CSV row for this cycle
  }

  Serial.println("=== DONE ===");
}

void loop() {}

// Drive every SRAM-facing pin LOW so nothing sources current into the chip
// through its I/O protection diodes while VDD is meant to be at 0.
void sramPinsLow() {
  digitalWrite(CE,  LOW);
  digitalWrite(OE,  LOW);
  digitalWrite(WE,  LOW);
  digitalWrite(BLE, LOW);
  digitalWrite(BHE, LOW);
  for (int i = 0; i < 18; i++) digitalWrite(ADDR[i], LOW);
  for (int i = 0; i < 16; i++) {
    pinMode(DATA[i], OUTPUT);   // ensure driven, not left as INPUT from a prior read
    digitalWrite(DATA[i], LOW);
  }
}

// Restore the default inactive bus state after power returns.
void sramDefaultBus() {
  digitalWrite(CE,  HIGH);   // deselected
  digitalWrite(OE,  HIGH);
  digitalWrite(WE,  HIGH);
  digitalWrite(BLE, LOW);
  digitalWrite(BHE, LOW);
}

void powerCycle() {
  // 0. Power on and write the gold pattern into the window BEFORE the off-window.
  sramPinsLow();                       // clean state before restoring power
  digitalWrite(PWR, PWR_ON);
  sramDefaultBus();
  delayMicroseconds(SETTLE_DELAY_US);

  for (uint32_t addr = 0; addr < NUM_WORDS; addr++) {
    sramWrite(addr, FILL);
  }

  // 1. Kill every sneak path BEFORE cutting power.
  sramPinsLow();

  // 2. Cut power for the decay window.
  digitalWrite(PWR, PWR_OFF);
  delayMicroseconds(OFF_DELAY_US);

  // 3. Restore power.
  digitalWrite(PWR, PWR_ON);

  // 4. Restore default bus state so the first read is clean.
  sramDefaultBus();
  delayMicroseconds(SETTLE_DELAY_US);
}

void setAddress(uint32_t addr) {
  for (int i = 0; i < 18; i++) {
    digitalWrite(ADDR[i], (addr >> i) & 1);
  }
}

void sramWrite(uint32_t addr, uint16_t val) {
  // data pins must be OUTPUT to drive the bus
  for (int i = 0; i < 16; i++) pinMode(DATA[i], OUTPUT);

  setAddress(addr);

  // put data on the bus
  for (int i = 0; i < 16; i++) {
    digitalWrite(DATA[i], (val >> i) & 1);
  }

  // WE-controlled write: CE low, OE high (don't drive the output during a write)
  digitalWrite(OE, HIGH);
  digitalWrite(CE, LOW);
  digitalWrite(WE, LOW);
  delayMicroseconds(5);
  digitalWrite(WE, HIGH);   // data latched on rising edge of WE
  digitalWrite(CE, HIGH);
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
