const int ledPin = 13; // led pin

const int jtag_clk = 22; // JTAG_CLK
const int jtag_tms = 24; // JTAG_TMS

const int jtag_tdi = 26; // JTAG_TDI
const int jtag_tdo = 28; // JTAG_TDO

void send_tms(size_t len, uint32_t data) {
  for (size_t i = 0; i < len; i++) {    
    uint8_t bit = data & 0x01;
    data >>= 1;
    digitalWrite(jtag_clk, LOW);
    digitalWrite(jtag_tms, bit ? HIGH : LOW);
    //delay(2);
    digitalWrite(jtag_clk, HIGH);
    //delay(2);
  }
}

void shift_data(size_t len, uint32_t* in_data, uint32_t* read_data, uint8_t tms_data) {
  digitalWrite(jtag_clk, LOW);
  //delay(10);
  for (size_t i = 0; i < len; i++) {
    uint8_t bit = *in_data & 0x01;
    *in_data >>= 1;      
    digitalWrite(jtag_tms, tms_data);
    digitalWrite(jtag_tdo, bit);    
    digitalWrite(jtag_clk, HIGH);
    //delay(2);    
    digitalWrite(jtag_clk, LOW);
    //delay(2);
    int val = digitalRead(jtag_tdi);
    *read_data >>= 1;
    *read_data |= (val << 7) << 24;
    //delay(2);
    //Serial.print(i);
    //Serial.print(" - ");
    //Serial.println(val);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);

  pinMode(jtag_clk, OUTPUT);
  pinMode(jtag_tms, OUTPUT);

  pinMode(jtag_tdi, INPUT);
  pinMode(jtag_tdo, OUTPUT);
}

void loop() {

  uint32_t in_data = 0x00;
  uint32_t read_data = 0x00;
  uint32_t tms_zero = 0x00;
  uint32_t tms_one = 0x01;
  
  /* 
  // TEST 1 - shift out IDCODE
  //
  send_tms(5, 0b11111); // reset to TEST_LOGIC_RESET
  send_tms(4, 0b0010); // to SHIFT_DR
  in_data = 0x87654321;
  read_data = 0x00;
  //shift_data(32, &in_data, &read_data, 0);
  shift_data(31, &in_data, &read_data, tms_zero);
  shift_data(1, &in_data, &read_data, tms_one);
  Serial.println(read_data, HEX);
 */

/*
  // TEST 2 - shift out the IR register (IDCODE: 0x0A0B0C0D)
  // at the same time shift in 0x0A0B0C0D and read it back
  //
  // reset to TEST_LOGIC_RESET
  send_tms(5, 0b11111); 
  // to SHIFT_IR
  send_tms(5, 0b00110); 
  // load SHIFT_IR with IDCODE of the custom register 1
  in_data = 0x0A0B0C0D;
  read_data = 0x00;
  shift_data(32, &in_data, &read_data, tms_zero);
  Serial.println(read_data, HEX);
  in_data = 0x00000000;
  read_data = 0x00;
  shift_data(32, &in_data, &read_data, tms_zero);
  Serial.println(read_data, HEX);
*/

/**/
  // TEST 3 - shift out the custom register 1 (IDCODE: 0x0A0B0C0D)
  //
  // reset to TEST_LOGIC_RESET
  send_tms(5, 0b11111); 
  // to SHIFT_IR
  send_tms(5, 0b00110); 
  // load SHIFT_IR with IDCODE of the custom register 1
  in_data = 0x0A0B0C0D;
  read_data = 0x00;
  shift_data(31, &in_data, &read_data, tms_zero);
  shift_data(1, &in_data, &read_data, tms_one); // on the last bit, transition to EXIT_IR by using a tms of 1
  // capture IR shift into IR data (transition over CAPTURE IR) and enter SHIFT_DR
  send_tms(6, 0b001110);
  // shift out the custom register 1, shift in 0A0B0C0D
  in_data = 0x0A0B0C0D;
  read_data = 0x00;
  shift_data(31, &in_data, &read_data, tms_zero);
  shift_data(1, &in_data, &read_data, tms_one);
  Serial.println(read_data, HEX);

  

  delay(3000);

}
