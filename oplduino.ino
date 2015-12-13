/**
 * Pin layout
 *
 * D0  (UNUSED)         -
 * D1  (UNUSED)         |
 * D2  (isa data 0)     |
 * D3  (isa data 1)     | PORTD
 * D4  (isa data 2)     | #####
 * D5  (isa data 3)     |
 * D6  (isa data 4)     |
 * D7  (isa data 5)     -

 * D8  (isa data 6)     -
 * D9  (isa data 7)     |
 * D10 (isa addr 0)     |
 * D11 (isa addr 1)     | PORTB
 * D12 (isa addr 2)     | #####
 * D13 (isa iowrite)    |
 * N/A (UNUSED)         |
 * N/A (UNUSED)         -
 */

void isa_write(unsigned int addr, unsigned char val) {
  // Set iowrite pin HIGH while we manipulate ports
  // (TODO: HIGH or LOW?)
  PORTB |= B00100000;

  // Manipulate temporary variables instead of the ports directly
  int b_temp = PORTB;
  int d_temp = PORTD;

  // Zero isa data & isa addr pins on PORTB
  b_temp &= B11100000;

  // Zero isa data pins on PORTD
  d_temp &= B00000011;

  /**
   * Address
   */
  // Only use the three least significant bits of the address
  addr &= B00000111;

  // Shift left twite to move addr bits past the data pins on PORTB
  addr << 2;

  // Write the isa address
  b_temp |= addr;

  /**
   * Data
   */
  // Invert the data (TODO: are we supposed to do this?)
  val = ~val;

  // Write data
  d_temp |= val << 2;
  b_temp |= val >> 6;

  // Update ports once we're ready
  PORTB = b_temp;
  PORTD = d_temp;

  // TODO: Measure the time between setting the values above and the latch
  // below; is it enough?

  // Set iowrite pin LOW to latch the data
  // (TODO: HIGH or LOW?)
  PORTB &= B11011111;
}

void setup() {
  // Set this as high as possible without data corruption
  Serial.begin(230400);

  // Set isa data, address & iowrite pins as outputs
  DDRD = DDRD | B11111100;
  DDRB = DDRB | B00111111;

  /**
   * Initialize mixer
   */
  // Reset mixer
  isa_write(0x204, 0x00); // Select reset register
  isa_write(0x205, 0x00); // Write anything to reset register

  /* Sound Blaster 2.0 */
  /*
  // Set master volume to 0 dB
  isa_write(0x204, 0x02); // Select master volume register
  isa_write(0x205, 0x0E); // Set volume to 0 dB

  // Set midi volume to 0 dB
  isa_write(0x204, 0x06); // Select midi volume register
  isa_write(0x205, 0x0E); // Set volume to 0 dB
  */

  /* Sound Blaster Pro */
  /*
  // Set master volume to 0 dB
  isa_write(0x204, 0x22); // Select master volume register
  isa_write(0x205, 0xEE); // Set L/R channels to 0 dB

  // Set midi volume to 0 dB
  isa_write(0x204, 0x26); // Select midi volume register
  isa_write(0x205, 0xEE); // Set L/R channels to 0 dB
  */

  /* Sound Blaster 16 */
  // Set master volume to 0 dB
  isa_write(0x204, 0x30); // Select master left volume register
  isa_write(0x205, 0xF8); // Set volume to 0 dB
  isa_write(0x204, 0x31); // Select master right volume register
  isa_write(0x205, 0xF8); // Set volume to 0 dB

  // Set midi volume to 0 dB
  isa_write(0x204, 0x34); // Select midi left volume register
  isa_write(0x205, 0xF8); // Set volume to 0 dB
  isa_write(0x204, 0x35); // Select midi right volume register
  isa_write(0x205, 0xF8); // Set volume to 0 dB
}

char hex_s[2];
byte data = 0;
byte io_addr = 0;

void loop() {
  while (true) {
    // Only the two least significant bits of the isa address are sent over
    // serial. Here we read bytes from the serial port until they are one of
    // these address values. (this simultaneously works as a sync signal)
    io_addr = Serial.read();
    if (io_addr >= 0 && io_addr <= 3) {
      break;
    }
  }

  // Data immediately follows the address.
  // Read two ASCII hex characters from serial
  Serial.readBytes(hex_s, 2);

  // Convert the hex string into a char
  data = strtol(hex_s, NULL, 16);

  isa_write(io_addr, data);
}
