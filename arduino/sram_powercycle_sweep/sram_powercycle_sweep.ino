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

// Power switch pin (drives gate of IRF4905 P-channel MOSFET)
// ASSUMPTION: HIGH = SRAM off, LOW = SRAM on (inverted, same as your existing FET logic).
// If this board's switch circuit is NOT inverted, flip PWR_OFF / PWR_ON below.
const int PWR = 53;
const int PWR_OFF = HIGH;
const int PWR_ON  = LOW;

// ---- Sweep configuration ----
// Off-hold durations to test, in microseconds.
// 50 (your reference point), then wider to see where flip-rate plateaus.
const uint32_t OFF_DELAYS_US[] = {50, 1000, 10000, 50000, 100000};
const int NUM_DELAYS = sizeof(OFF_DELAYS_US) / sizeof(OFF_DELAYS_US[0]);

const int CYCLES_PER_DELAY = 50;   // power cycles to run at each delay setting
const uint32_t NUM_WORDS = 100;    // same address range each time, for fair comparison
const uint32_t SETTLE_DELAY_US = 5000; // time to let SRAM stabilize after power-on, before reading

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Power switch
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, PWR_OFF); // start powered off

  // Control pins
  pinMode(CE,  OUTPUT); digitalWrite(CE,  HIGH);
  pinMode(OE,  OUTPUT); digitalWrite(OE,  HIGH);
  pinMode(WE,  OUTPUT); digitalWrite(WE,  HIGH);
  pinMode(BLE, OUTPUT); digitalWrite(BLE, LOW);
  pinMode(BHE, OUTPUT); digitalWrite(BHE, LOW);

  // Address pins
  for (int i = 0; i < 18; i++) pinMode(ADDR[i], OUTPUT);

  Serial.println("=== SRAM POWER-CYCLE SWEEP ===");
  Serial.println("Format: DELAY_US,CYCLE,WORD,BITS");
  Serial.println();

  for (int d = 0; d < NUM_DELAYS; d++) {
    uint32_t offDelay = OFF_DELAYS_US[d];

    for (int cycle = 0; cycle < CYCLES_PER_DELAY; cycle++) {
      powerCycle(offDelay);

      for (uint32_t addr = 0; addr < NUM_WORDS; addr++) {
        uint16_t val = sramRead(addr);

        Serial.print(offDelay);
        Serial.print(",");
        Serial.print(cycle);
        Serial.print(",");
        Serial.print(addr);
        Serial.print(",");
        for (int b = 15; b >= 0; b--) {
          Serial.print((val >> b) & 1);
        }
        Serial.println();
      }
    }
  }

  Serial.println();
  Serial.println("=== DONE ===");
}

void loop() {}

void powerCycle(uint32_t offDelayUs) {
  // Make sure control lines aren't actively driving the bus while power toggles
  digitalWrite(CE, HIGH);
  digitalWrite(OE, HIGH);
  digitalWrite(WE, HIGH);

  digitalWrite(PWR, PWR_OFF);
  delayMicroseconds(offDelayUs);

  digitalWrite(PWR, PWR_ON);
  delayMicroseconds(SETTLE_DELAY_US); // let rail/SRAM stabilize before reading startup state
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
