#include <Wire.h>

// ============================================================
// STM32 BLUE PILL AS5600 PROGRAMMER / BURNER
// ============================================================
//
// Purpose:
// Permanently program an AS5600 with:
//
//     ZPOS = 2787  (~245 degrees)
//     MPOS = 3356  (~295 degrees)
//
// The same firmware automatically detects the AS5600 on either:
//
//     I2C1: PB6 = SCL, PB7 = SDA
//     I2C2: PB10 = SCL, PB11 = SDA
//
// No magnet is required during programming.
//
// PA1:
//     Normally left floating/high.
//     Connect PA1 to GND to trigger the permanent burn.
//
// PC13:
//     Onboard Blue Pill LED, active LOW.
//
// Wiring:
//
// BLUE PILL       AS5600
// ---------       ------
// 3.3V     ->     VCC
// GND      ->     GND
//
// I2C1:
// PB6      ->     SCL
// PB7      ->     SDA
//
// OR
//
// I2C2:
// PB10     ->     SCL
// PB11     ->     SDA
//
// ============================================================


// ------------------------------------------------------------
// AS5600
// ------------------------------------------------------------

const uint8_t AS5600_ADDR = 0x36;


// ------------------------------------------------------------
// Pins
// ------------------------------------------------------------

const uint8_t BURN_PIN = PA1;
const uint8_t LED_PIN  = PC13;


// ------------------------------------------------------------
// HARD-CODED ANGLE RANGE
// ------------------------------------------------------------

const uint16_t START_VAL = 2787;
const uint16_t END_VAL   = 3356;


// ------------------------------------------------------------
// AS5600 REGISTERS
// ------------------------------------------------------------

const uint8_t REG_STATUS  = 0x0B;
const uint8_t REG_ANGLE   = 0x0C;
const uint8_t REG_RAW     = 0x0E;

const uint8_t REG_ZPOS    = 0x01;
const uint8_t REG_MPOS    = 0x03;

const uint8_t REG_BURN    = 0xFF;

const uint8_t BURN_ANGLE  = 0x80;


// ------------------------------------------------------------
// STATUS REGISTER
//
// STATUS bit 5 = MD
// MD = Magnet Detected
//
// We use this ONLY to suppress useless angle printing.
// It does NOT prevent programming or burning.
// ------------------------------------------------------------

const uint8_t STATUS_MD = 0x20;


// ------------------------------------------------------------
// I2C buses
//
// STM32duino TwoWire constructor:
// TwoWire(SDA, SCL)
// ------------------------------------------------------------

TwoWire Wire1(PB7, PB6);
TwoWire Wire2(PB11, PB10);


// Pointer to whichever bus contains the AS5600
TwoWire* AS5600_Wire = nullptr;


// ------------------------------------------------------------
// Timing
// ------------------------------------------------------------

unsigned long lastPrint = 0;


// ============================================================
// READ 8-BIT REGISTER
// ============================================================

bool readReg8(uint8_t reg, uint8_t &value)
{
  if (AS5600_Wire == nullptr)
    return false;

  AS5600_Wire->beginTransmission(AS5600_ADDR);
  AS5600_Wire->write(reg);

  if (AS5600_Wire->endTransmission(false) != 0)
    return false;

  uint8_t count = AS5600_Wire->requestFrom(AS5600_ADDR, (uint8_t)1);

  if (count != 1)
    return false;

  value = AS5600_Wire->read();

  return true;
}


// ============================================================
// READ 16-BIT REGISTER
//
// AS5600 registers are 12-bit values stored as:
//
//     HIGH BYTE
//     LOW BYTE
//
// Only the lower 12 bits are used.
// ============================================================

bool readReg16(uint8_t reg, uint16_t &value)
{
  if (AS5600_Wire == nullptr)
    return false;

  AS5600_Wire->beginTransmission(AS5600_ADDR);
  AS5600_Wire->write(reg);

  if (AS5600_Wire->endTransmission(false) != 0)
    return false;

  uint8_t count = AS5600_Wire->requestFrom(AS5600_ADDR, (uint8_t)2);

  if (count != 2)
    return false;

  uint8_t highByte = AS5600_Wire->read();
  uint8_t lowByte  = AS5600_Wire->read();

  value = ((uint16_t)highByte << 8) | lowByte;

  value &= 0x0FFF;

  return true;
}


// ============================================================
// WRITE 16-BIT REGISTER
// ============================================================

bool writeReg16(uint8_t reg, uint16_t value)
{
  if (AS5600_Wire == nullptr)
    return false;

  value &= 0x0FFF;

  AS5600_Wire->beginTransmission(AS5600_ADDR);

  AS5600_Wire->write(reg);

  // High byte
  AS5600_Wire->write((value >> 8) & 0x0F);

  // Low byte
  AS5600_Wire->write(value & 0xFF);

  uint8_t error = AS5600_Wire->endTransmission();

  if (error != 0)
    return false;

  // Give AS5600 time to process the write
  delay(5);

  return true;
}


// ============================================================
// DETECT AS5600 ON A BUS
//
// We don't require a magnet.
//
// We simply attempt to read a valid register.
// ============================================================

bool detectAS5600(TwoWire &bus)
{
  bus.begin();
  bus.setClock(400000);

  bus.beginTransmission(AS5600_ADDR);

  if (bus.endTransmission() != 0)
    return false;

  // Make sure we can actually read from the device.
  bus.beginTransmission(AS5600_ADDR);
  bus.write(REG_RAW);

  if (bus.endTransmission(false) != 0)
    return false;

  uint8_t count = bus.requestFrom(AS5600_ADDR, (uint8_t)2);

  if (count != 2)
    return false;

  bus.read();
  bus.read();

  return true;
}


// ============================================================
// FIND AS5600
//
// First try I2C1.
// Then try I2C2.
// ============================================================

bool findAS5600()
{
  Serial.println();
  Serial.println(F("Searching for AS5600..."));

  Serial.println(F("Checking I2C1: PB6=SCL, PB7=SDA"));

  if (detectAS5600(Wire1))
  {
    AS5600_Wire = &Wire1;

    Serial.println(F("AS5600 FOUND on I2C1."));
    return true;
  }

  Serial.println(F("Not found on I2C1."));

  Serial.println(F("Checking I2C2: PB10=SCL, PB11=SDA"));

  if (detectAS5600(Wire2))
  {
    AS5600_Wire = &Wire2;

    Serial.println(F("AS5600 FOUND on I2C2."));
    return true;
  }

  Serial.println(F("AS5600 NOT FOUND."));

  return false;
}


// ============================================================
// SHOW ANGLE
//
// IMPORTANT:
//
// If there is no magnet, nothing is printed.
//
// This is purely informational and has NO effect on burning.
// ============================================================

void printAngleIfMagnetPresent()
{
  if (millis() - lastPrint < 500)
    return;

  lastPrint = millis();

  uint8_t status;

  if (!readReg8(REG_STATUS, status))
    return;

  // No magnet -> don't print angle
  if (!(status & STATUS_MD))
    return;

  uint16_t rawAngle;

  if (!readReg16(REG_RAW, rawAngle))
    return;

  rawAngle &= 0x0FFF;

  Serial.print(F("Magnet detected - Raw angle: "));
  Serial.println(rawAngle);
}


// ============================================================
// WRITE AND VERIFY ZPOS / MPOS
// ============================================================

bool programAngleRange()
{
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("PROGRAMMING AS5600"));
  Serial.println(F("========================================"));

  Serial.print(F("ZPOS = "));
  Serial.println(START_VAL);

  Serial.print(F("MPOS = "));
  Serial.println(END_VAL);

  Serial.println();

  // ----------------------------------------------------------
  // Write ZPOS
  // ----------------------------------------------------------

  Serial.println(F("Writing ZPOS..."));

  if (!writeReg16(REG_ZPOS, START_VAL))
  {
    Serial.println(F("ERROR: ZPOS write failed."));
    return false;
  }

  Serial.println(F("ZPOS write OK."));


  // ----------------------------------------------------------
  // Write MPOS
  // ----------------------------------------------------------

  Serial.println(F("Writing MPOS..."));

  if (!writeReg16(REG_MPOS, END_VAL))
  {
    Serial.println(F("ERROR: MPOS write failed."));
    return false;
  }

  Serial.println(F("MPOS write OK."));


  // ----------------------------------------------------------
  // Read back ZPOS
  // ----------------------------------------------------------

  uint16_t zposRead;

  if (!readReg16(REG_ZPOS, zposRead))
  {
    Serial.println(F("ERROR: Could not read ZPOS back."));
    return false;
  }

  Serial.print(F("ZPOS readback = "));
  Serial.println(zposRead);

  if (zposRead != START_VAL)
  {
    Serial.println(F("ERROR: ZPOS verification FAILED."));
    return false;
  }


  // ----------------------------------------------------------
  // Read back MPOS
  // ----------------------------------------------------------

  uint16_t mposRead;

  if (!readReg16(REG_MPOS, mposRead))
  {
    Serial.println(F("ERROR: Could not read MPOS back."));
    return false;
  }

  Serial.print(F("MPOS readback = "));
  Serial.println(mposRead);

  if (mposRead != END_VAL)
  {
    Serial.println(F("ERROR: MPOS verification FAILED."));
    return false;
  }


  Serial.println();
  Serial.println(F("ZPOS verification OK."));
  Serial.println(F("MPOS verification OK."));
  Serial.println(F("RAM values are correct."));

  return true;
}


// ============================================================
// WAIT FOR PA1 TO BE GROUNDED
// ============================================================

void waitForBurnTrigger()
{
  Serial.println();
  Serial.println(F("----------------------------------------"));
  Serial.println(F("READY TO BURN"));
  Serial.println(F("----------------------------------------"));
  Serial.println(F("Connect PA1 to GND to permanently burn."));
  Serial.println(F("Magnet is NOT required."));
  Serial.println();

  // Wait until PA1 is grounded.
  while (digitalRead(BURN_PIN) == HIGH)
  {
    printAngleIfMagnetPresent();
    delay(10);
  }

  // Require PA1 to remain LOW briefly.
  delay(100);

  // Confirm it is still grounded.
  if (digitalRead(BURN_PIN) == LOW)
  {
    Serial.println();
    Serial.println(F("BURN TRIGGER DETECTED."));
  }
}


// ============================================================
// PERFORM OTP BURN
// ============================================================

bool burnAngle()
{
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("SENDING BURN_ANGLE COMMAND"));
  Serial.println(F("========================================"));

  Serial.println(F("Writing 0x80 to register 0xFF..."));

  AS5600_Wire->beginTransmission(AS5600_ADDR);

  AS5600_Wire->write(REG_BURN);
  AS5600_Wire->write(BURN_ANGLE);

  uint8_t error = AS5600_Wire->endTransmission();

  if (error != 0)
  {
    Serial.print(F("ERROR: Burn command I2C error = "));
    Serial.println(error);

    return false;
  }

  delay(10);

  Serial.println(F("BURN_ANGLE command sent."));
  Serial.println(F("The AS5600 OTP programming operation has been requested."));

  return true;
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  // LED
  pinMode(LED_PIN, OUTPUT);

  // LED OFF
  digitalWrite(LED_PIN, HIGH);


  // PA1 uses internal pull-up.
  //
  // Normally:
  //     PA1 = HIGH
  //
  // Burn:
  //     PA1 -> GND
  //
  pinMode(BURN_PIN, INPUT_PULLUP);


  // Serial
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("STM32 AS5600 PROGRAMMER"));
  Serial.println(F("========================================"));

  Serial.print(F("Target ZPOS: "));
  Serial.println(START_VAL);

  Serial.print(F("Target MPOS: "));
  Serial.println(END_VAL);

  Serial.println();
  Serial.println(F("Magnet is NOT required."));
  Serial.println(F("Angle output is suppressed when no magnet"));
  Serial.println(F("is detected."));
  Serial.println();


  // ----------------------------------------------------------
  // Find AS5600
  // ----------------------------------------------------------

  if (!findAS5600())
  {
    Serial.println();
    Serial.println(F("STOPPED."));
    Serial.println(F("Check VCC, GND, SDA and SCL."));

    // Fast blink = no AS5600 found
    while (true)
    {
      digitalWrite(LED_PIN, LOW);
      delay(150);
      digitalWrite(LED_PIN, HIGH);
      delay(150);
    }
  }


  // ----------------------------------------------------------
  // AS5600 detected
  // ----------------------------------------------------------

  digitalWrite(LED_PIN, LOW);

  Serial.println();
  Serial.println(F("AS5600 communication established."));


  // ----------------------------------------------------------
  // Program ZPOS / MPOS
  // ----------------------------------------------------------

  if (!programAngleRange())
  {
    Serial.println();
    Serial.println(F("PROGRAMMING FAILED."));
    Serial.println(F("OTP burn has NOT been attempted."));

    // Error indication
    while (true)
    {
      digitalWrite(LED_PIN, LOW);
      delay(500);
      digitalWrite(LED_PIN, HIGH);
      delay(500);
    }
  }


  // ----------------------------------------------------------
  // Wait for physical burn trigger
  // ----------------------------------------------------------

  waitForBurnTrigger();


  // ----------------------------------------------------------
  // Burn
  // ----------------------------------------------------------

  if (burnAngle())
  {
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("BURN COMMAND COMPLETE"));
    Serial.println(F("========================================"));
    Serial.println(F("Remove PA1 from GND."));
    Serial.println(F("Programming sequence finished."));
  }
  else
  {
    Serial.println();
    Serial.println(F("BURN COMMAND FAILED."));
  }


  // Turn LED off when finished
  digitalWrite(LED_PIN, HIGH);

  // Stop here.
  while (true)
  {
    delay(1000);
  }
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // Everything is handled in setup().
  // Nothing further is required.
}
