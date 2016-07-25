#include <SPI.h>

#define PIN_I1 A0
#define PIN_I2 A1
#define PIN_I3 A2
#define PIN_I4 A3

#define PIN_INB4 A5
#define PIN_INB3 0
#define PIN_INB2 1
#define PIN_INB1 2
#define PIN_INH4 3
#define PIN_INA3 4
#define PIN_INH2 5
#define PIN_INH1 6
#define PIN_INA2 7
//#define PIN_INA4 //PB6, which is not accessible from arduino (because it's XTAL1)
#define PIN_INA1 8
#define PIN_INH3 9

#define PIN_ADDR0 A4
#define PIN_ADDR1 10
#define PIN_DATA_IN 11
#define PIN_DATA_OUT 12
#define PIN_DATA_CLOCK 13
//#define PIN_EM_ACK //PB7, s.a

byte bufOut[6] = { 0x00, 0x00, 0x00, 110 /*330/3*/, 0x00, 0x00 };
byte bufIn[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
volatile byte pos;
byte debugDataOut = 0;

void setup() {
  pinMode(PIN_I1, INPUT);
  pinMode(PIN_I2, INPUT);
  pinMode(PIN_I3, INPUT);
  pinMode(PIN_I4, INPUT);

  pinMode(PIN_INA1, OUTPUT);
  pinMode(PIN_INA2, OUTPUT);
  pinMode(PIN_INA3, OUTPUT);
  DDRB |= bit(6); //INA4 is OUTPUT
  pinMode(PIN_INB1, OUTPUT);
  pinMode(PIN_INB2, OUTPUT);
  pinMode(PIN_INB3, OUTPUT);
  pinMode(PIN_INB4, OUTPUT);
  pinMode(PIN_INH1, OUTPUT);
  pinMode(PIN_INH2, OUTPUT);
  pinMode(PIN_INH3, OUTPUT);
  pinMode(PIN_INH4, OUTPUT);

  pinMode(PIN_ADDR0, INPUT_PULLUP);
  //pinMode(PIN_ADDR1, INPUT_PULLUP); //WARNING: This is slave select, it must be low (HW bug!)
  /* I currently dont see a way to fix this in software. The !SS pin must be externally pulled
   * down in slave mode, so we loose this pin completly. In therory it would be enough to pull
   * it down after the address was detected, but we don't have a matching pin for that either.
   * Pulling this in down is easy in hardware, but we'll loose the ability to read ADDR1, and
   * therefore can listen only on one address (or on all even addresses to be more preceise).
   * We just have not enought pins :(
  */
  DDRB |= bit(7); //EM_ACK is OUTPUT

  pinMode(PIN_DATA_IN, INPUT_PULLUP);
  pinMode(PIN_DATA_OUT, OUTPUT);
  pinMode(PIN_DATA_CLOCK, INPUT_PULLUP);
  pos = 0;

  SPI.setDataMode(SPI_MODE3);
  SPI.attachInterrupt();
}

ISR (SPI_STC_vect) //SPI interrupt
{
  //read data from register
  bufIn[pos] = ~SPDR;
  //put next value in the register
  pos++;
  SPDR = ~bufOut[pos];

  digitalWriteEmAck(HIGH);
  delayMicroseconds(2);
  digitalWriteEmAck(LOW);
}

void loop() {
  //wait for address
  while (digitalRead(PIN_ADDR0) == HIGH) {}

  //reset SPI module
  SPCR &= ~bit(SPE);
  SPCR |= bit(SPE);
  pos = 0;

  //read in data
  bufOut[0] = (digitalRead(PIN_I1) << 0) | (digitalRead(PIN_I2) << 1) | (digitalRead(PIN_I3) << 2) | (digitalRead(PIN_I4) << 3) | (debugDataOut & 0xF) << 4;
  //TODO: Analog values

  //put first byte in register
  SPDR = ~bufOut[0];

  delayMicroseconds(100); //700µs delay in original module, not sure if needed
  digitalWriteEmAck(LOW);

  while (digitalRead(PIN_ADDR0) == LOW) {} //wait for master to release address

  delayMicroseconds(10);
  digitalWriteEmAck(HIGH);

  //process in data
  digitalWrite(PIN_INA1, (bufIn[1] & bit(0)) != 0);
  digitalWrite(PIN_INB1, (bufIn[1] & bit(1)) != 0);
  digitalWrite(PIN_INA2, (bufIn[1] & bit(2)) != 0);
  digitalWrite(PIN_INB2, (bufIn[1] & bit(3)) != 0);
  digitalWrite(PIN_INA3, (bufIn[1] & bit(4)) != 0);
  digitalWrite(PIN_INB3, (bufIn[1] & bit(5)) != 0);
  digitalWriteInA4((bufIn[1] & bit(6)) != 0);
  digitalWrite(PIN_INB4, (bufIn[1] & bit(7)) != 0);

  //we ignore the even speeds, this will be the same as the odd due to hardware limitations. Maybe we can do some clever tricks to fix that...
  byte speed1 = bufIn[2] & 0x7;
  byte speed3 = ((bufIn[3] & 0x1) << 2) | ((bufIn[2] >> 6) & 0x3); //Yay, bit magic...
  byte speed5 = (bufIn[3] >> 4) & 0x7;
  byte speed7 = (bufIn[4] >> 2) & 0x7;
  
  analogWrite(PIN_INH1, speed1 << 5);
  analogWrite(PIN_INH2, speed3 << 5);
  analogWrite(PIN_INH3, speed5 << 5);
  analogWrite(PIN_INH4, speed7 << 5);
  
  delay(1);
}

void digitalWriteEmAck(byte val) {
  if (val == LOW)
    PORTB &= ~bit(7);
  else if (val == HIGH)
    PORTB |= bit(7);
}

void digitalWriteInA4(byte val) {
  if (val == LOW)
    PORTB &= ~bit(6);
  else if (val == HIGH)
    PORTB |= bit(6);
}

