// SRAM decay-to-natural-state test.
//
// PHASE 1 (gold reference): power up cold from a long off-window, read WITHOUT
//   writing. This captures the array's natural power-up fingerprint. Repeated
//   REF_CYCLES times so Python can majority-vote a clean reference.
//   Rows marked "# ref N".
//
// PHASE 2 (test): write all 1s, power off for OFF_DELAY_US, power on, read.
//   Rows marked "# cycle N".
//
// Metric (computed in Python): Hamming distance between each test readback and
//   the gold reference. Large distance = written 1s still surviving; distance
//   shrinking toward the unstable-bit floor = array has decayed back to its
//   natural power-up state. Sweep OFF_DELAY_US to find the time it takes.

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

const uint32_t NUM_WORDS = 1000;
const uint32_t SETTLE_DELAY_US = 5000;      // 5 ms to stabilize after power-on

// --- Phase 1: gold reference (natural cold-start) ---
const int REF_CYCLES = 1;                  // reads to majority-vote a clean reference
const uint32_t REF_OFF_DELAY_US = 3000000;  // 3 s: well past retention, guarantees cold start

// --- Phase 2: write-1s decay test ---
const int NUM_CYCLES = 1;
const uint16_t FILL = 0xFFFF;               // pattern written before each off-window
const uint32_t OFF_DELAY_US = 1750000;      // sweep THIS to trace decay-to-natural

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

  // Data pins -- OUTPUT so we can force them LOW during the off-window
  for (int i = 0; i < 16; i++) pinMode(DATA[i], OUTPUT);

  Serial.print("=== SRAM DECAY-TO-NATURAL TEST: ref_cycles=");
  Serial.print(REF_CYCLES);
  Serial.print(", test_cycles=");
  Serial.print(NUM_CYCLES);
  Serial.print(", off_delay=");
  Serial.print(OFF_DELAY_US / 1000);
  Serial.print("ms, fill=0x");
  if (FILL < 0x1000) Serial.print("0");
  if (FILL < 0x0100) Serial.print("0");
  if (FILL < 0x0010) Serial.print("0");
  Serial.print(FILL, HEX);
  Serial.println(" ===");
  Serial.println();

  // ---------- PHASE 1: gold reference (no write) ----------
  for (int r = 0; r < REF_CYCLES; r++) {
    coldStart();                 // long off-window, power on, no write
    Serial.print("# ref ");
    Serial.println(r);
    dumpWindow();
  }

  // ---------- PHASE 2: write-1s decay test ----------
  for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
    writeDecayCycle();           // write FILL, off for OFF_DELAY_US, power on
    Serial.print("# cycle ");
    Serial.println(cycle);
    dumpWindow();
  }

  Serial.println("=== DONE ===");
}

void loop() {}

// Read the whole window and print it as one comma-separated hex row.
void dumpWindow() {
  for (uint32_t addr = 0; addr < NUM_WORDS; addr++) {
    uint16_t val = sramRead(addr);
    if (val < 0x1000) Serial.print("0");
    if (val < 0x0100) Serial.print("0");
    if (val < 0x0010) Serial.print("0");
    Serial.print(val, HEX);
    if (addr < NUM_WORDS - 1) Serial.print(",");
  }
  Serial.println();
}

// Phase 1: fully decay, then power on cold. No write -> natural power-up state.
void coldStart() {
  sramPinsLow();
  digitalWrite(PWR, PWR_OFF);
  delayMicroseconds(REF_OFF_DELAY_US);
  digitalWrite(PWR, PWR_ON);
  sramDefaultBus();
  delayMicroseconds(SETTLE_DELAY_US);
}

// Phase 2: power on, write FILL, then off for OFF_DELAY_US, then power on.
void writeDecayCycle() {
  // power on and write the forced pattern
  sramPinsLow();
  digitalWrite(PWR, PWR_ON);
  sramDefaultBus();
  delayMicroseconds(SETTLE_DELAY_US);

  for (uint32_t addr = 0; addr < NUM_WORDS; addr++) {
    sramWrite(addr, FILL);
  }

  // kill sneak paths, cut power for the decay window
  sramPinsLow();
  digitalWrite(PWR, PWR_OFF);
  delayMicroseconds(OFF_DELAY_US);

  // restore power, clean bus for the read
  digitalWrite(PWR, PWR_ON);
  sramDefaultBus();
  delayMicroseconds(SETTLE_DELAY_US);
}

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
    pinMode(DATA[i], OUTPUT);
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

void setAddress(uint32_t addr) {
  for (int i = 0; i < 18; i++) {
    digitalWrite(ADDR[i], (addr >> i) & 1);
  }
}

void sramWrite(uint32_t addr, uint16_t val) {
  for (int i = 0; i < 16; i++) pinMode(DATA[i], OUTPUT);

  setAddress(addr);

  for (int i = 0; i < 16; i++) {
    digitalWrite(DATA[i], (val >> i) & 1);
  }

  // WE-controlled write: CE low, OE high (don't drive output during a write)
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
