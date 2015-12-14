// uncomment this line to run test sequence,
// should generate a test tone from sound card
#define TEST_TONE

// Base ISA address of the OPL3 chip
#define OPL3_BASE 0x220

//#define SB2
//#define SBPRO
#define SB16

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
 * 
 * A0  (isa ioread)     - PORTC
 */

char debug_s[32];

void isa_read(unsigned int addr) {
  // Make sure iowrite pin is HIGH
  PORTB |= B00100000;
  // Make sure ioread pin is HIGH
  PORTC |= B00000001;

  // Manipulate temporary variables instead of the ports directly
  int b_temp = PORTB;
  int d_temp = PORTD;
  
  // Zero isa data & isa addr pins on PORTB (set data pins to 1 as data is inverted)
  b_temp &= B11100000;
  //b_temp &= B11100011;
  //b_temp |= B00000011;

  // Zero isa data pins on PORTD (set to 1 as data is inverted)
  d_temp &= B00000011;
  //d_temp |= B11111100;

  /**
   * Address
   */

  // Only use the three least significant bits of the address
  addr &= B00000111;

  // Shift left twice to move addr bits past the two data bits on PORTB
  addr = addr << 2;

  // Write the isa address
  b_temp |= addr;

  // Set the address and zero data
  PORTB = b_temp;
  PORTD = d_temp;
  
  // Set ioread pin LOW to initiate read
  // Two Arduino cycles needed to meet 150 ns (but is this only relevant when writing directly to OPL3?)
  PORTC &= B11111110;
  PORTC &= B11111110;

  // Set ioread pin HIGH; we're done reading
  PORTC |= B00000001;
}

void do_6_reads() {
  for(int i = 0; i < 3; i++) {
    isa_read(OPL3_BASE + 0);
    isa_read(OPL3_BASE + 2);
  }
}

void isa_write(unsigned int addr, unsigned char val) {
  #ifdef TEST_TONE
  //snprintf(debug_s, 32, "%02x, %02x", addr, val);
  //Serial.println(debug_s);
  #endif

  // Make sure ioread pin is HIGH
  PORTC |= B00000001;
  
  // Set iowrite pin HIGH while we manipulate ports
  // (TODO: HIGH or LOW?)
  PORTB |= B00100000;

  // Manipulate temporary variables instead of the ports directly
  int b_temp = PORTB;
  int d_temp = PORTD;

  //delay(3);

  // Zero isa data & isa addr pins on PORTB
  b_temp &= B11100000;

  // Zero isa data pins on PORTD
  d_temp &= B00000011;

  /**
   * Address
   */
  // Only use the three least significant bits of the address
  addr &= B00000111;  

  // Shift left twice to move addr bits past the two data bits on PORTB
  addr = addr << 2;

  // Write the isa address
  b_temp |= addr;

  /**
   * Data
   */
  // Invert the data (TODO: are we supposed to do this?)
  //val = ~val;

  // Write data
  d_temp |= val << 2;
  b_temp |= val >> 6;

  // Update ports once we're ready
  PORTB = b_temp;
  PORTD = d_temp;

  // TODO: Measure the time between setting the values above and the latch
  // below; is it enough?

  delayMicroseconds(10);

  // Set iowrite pin LOW to latch the data
  // (TODO: HIGH or LOW?)
  PORTB &= B11011111;

  delayMicroseconds(10);
}

// Helper function for writing a value to both left/right OPL3
// registers
void opl_write_stereo(unsigned char reg, unsigned char val) {
    isa_write(OPL3_BASE + 0, reg);
    do_6_reads();
    isa_write(OPL3_BASE + 1, val);
    do_6_reads();
    isa_write(OPL3_BASE + 2, reg);
    do_6_reads();
    isa_write(OPL3_BASE + 3, val);
    do_6_reads();
}

void setup() {
  // Set this as high as possible without data corruption
  //Serial.begin(230400);
  Serial.begin(115200);

  // Set isa data, address & iowrite/read pins as outputs
  DDRD = DDRD | B11111100;
  DDRB = DDRB | B00111111;
  DDRC = DDRC | B00000001;

  /**
   * Initialize mixer
   */
  // For mixer documentation, see chapter 4 of:
  // https://pdos.csail.mit.edu/6.828/2014/readings/hardware/SoundBlaster.pdf

  // Reset mixer
  isa_write(0x224, 0x00); // Select reset register
  isa_write(0x225, 0x00); // Write anything to reset register

  /* Sound Blaster 2.0 */
  #ifdef SB2
  // Set master volume to 0 dB
  isa_write(0x224, 0x02); // Select master volume register
  isa_write(0x225, 0x0E); // Set volume to 0 dB

  // Set midi volume to 0 dB
  isa_write(0x224, 0x06); // Select midi volume register
  isa_write(0x225, 0x0E); // Set volume to 0 dB
  #endif

  /* Sound Blaster Pro */
  #ifdef SBPRO
  // Set master volume to 0 dB
  isa_write(0x224, 0x22); // Select master volume register
  isa_write(0x225, 0xEE); // Set L/R channels to 0 dB

  // Set midi volume to 0 dB
  isa_write(0x224, 0x26); // Select midi volume register
  isa_write(0x225, 0xEE); // Set L/R channels to 0 dB
  #endif

  /* Sound Blaster 16 */
  #ifdef SB16
  // Set master volume to 0 dB
  isa_write(0x224, 0x30); // Select master left volume register
  isa_write(0x225, 0xF8); // Set volume to 0 dB
  isa_write(0x224, 0x31); // Select master right volume register
  isa_write(0x225, 0xF8); // Set volume to 0 dB

  // Set midi volume to 0 dB
  isa_write(0x224, 0x34); // Select midi left volume register
  isa_write(0x225, 0xF8); // Set volume to 0 dB
  isa_write(0x224, 0x35); // Select midi right volume register
  isa_write(0x225, 0xF8); // Set volume to 0 dB
  #endif
}

char hex_s[2];
byte data = 0;
byte io_addr = 0;

void loop() {
  #ifdef TEST_TONE

  // Test mode: play a simple tone to test the setup
  // See the "Making a Sound" section of:
  // http://www.floodgap.com/retrobits/ckb/secret/adlib.txt

  // Start by zeroing out every register of the OPL3.
  // "Quick-and-dirty method of resetting the sound card,
  // but it works", according to above guide
  for (int i = 0; i <= 0xFF; i++) {
      opl_write_stereo(i, 0x00);
  }

  //isa_write(0x220, 0x01);

  /*
  // Set the modulator's multiple to 1
  opl_write_stereo(0x20, 0x01);
  // Set the modulator's level to about 40 dB
  opl_write_stereo(0x40, 0x10);
  // Modulator attack:  quick;   decay:   long
  opl_write_stereo(0x60, 0xF0);
  // Modulator sustain: medium;  release: medium
  opl_write_stereo(0x80, 0x77);
  // Set voice frequency's LSB (it'll be a D#)
  opl_write_stereo(0xA0, 0x98);
  // Set the carrier's multiple to 1
  opl_write_stereo(0x23, 0x01);
  // Set the carrier to maximum volume (about 47 dB)
  opl_write_stereo(0x43, 0x00);
  // Carrier attack:  quick;   decay:   long
  opl_write_stereo(0x63, 0xF0);
  // Carrier sustain: medium;  release: medium
  opl_write_stereo(0x83, 0x77);
  // Turn the voice on; set the octave and freq MSB
  opl_write_stereo(0xB0, 0x31);
  */

  // readout from logic analyzer
  opl_write_stereo(0x68, 0xFE);
  opl_write_stereo(0x88, 0x3F);
  opl_write_stereo(0xE0, 0x02);
  opl_write_stereo(0x6B, 0xF0);
  opl_write_stereo(0x8B, 0xFF);
  opl_write_stereo(0xEB, 0x00);
  opl_write_stereo(0x28, 0x60);
  opl_write_stereo(0x2B, 0x42);
  opl_write_stereo(0xC3, 0x3E);
  opl_write_stereo(0x48, 0x0B);
  opl_write_stereo(0x4B, 0x1D);
  opl_write_stereo(0xB3, 0x00);
  opl_write_stereo(0xB3, 0x2D);
  opl_write_stereo(0xA3, 0x81);
  opl_write_stereo(0x48, 0x0B);
  opl_write_stereo(0x4B, 0x1D);
  

  // wait a second
  delay(1000);
  //delayMicroseconds(1);

  #else

  // Normal operation: receive OPL3 commands over serial port
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

  isa_write(OPL3_BASE + io_addr, data);

  #endif
}
