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
  // STEPS: 
  // 1. flash the JTAG verilog project onto the GOWIN FPGA because on power on it has no application loaded initially
  // 2. Wire up the systems and run the application.
  // 
  // EXPECTATION: the ID Code of the device is printed continuously
  //
  // reset to TEST_LOGIC_RESET
  send_tms(5, 0b11111);
  // to SHIFT_DR
  send_tms(4, 0b0010);
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
  // EXPECTATION: the value 0A0B0C0D is printed continuously
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

/*
  // TEST 3 - shift out the custom register 1 (IDCODE: 0x0A0B0C0D)
  //
  // EXPECTATION: the value 0x12345678 is printed continuously
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
  // fill custom register with data 32 bit data (0x12345678) and stay in SHIFT_DR
  in_data = 0x12345678;
  read_data = 0x00;
  shift_data(32, &in_data, &read_data, tms_zero);
  // shift out the custom register 1, shift in 0
  in_data = 0x00;
  read_data = 0x00;
  shift_data(31, &in_data, &read_data, tms_zero);
  shift_data(1, &in_data, &read_data, tms_one);
  Serial.println(read_data, HEX);
*/
  
/**/
  // Test 4 - reading the BYPASS register, to count devices
  //
  printf("TEST 4 - Start ...\n"); delay(200);

  // reset to TEST_LOGIC_RESET
  send_tms(5, 0b11111); 

  delay(4000);
  
  // to SHIFT_IR
  send_tms(5, 0b00110);
  printf("TEST 4 - A\n"); delay(200);

  delay(4000);
  
  // Start to load 1s into all instruction registers of all devices in the entire JTAG chain because an IR register filled with
  // all 1s is the BYPASS instruction.
  for (int i = 0; i < 9; i++) {
    
    printf("TEST 4 - B\n"); delay(200);
    in_data = 0xFFFFFFFF;
    read_data = 0x00;
    shift_data(32, &in_data, &read_data, tms_zero);

    delay(1000);
  }
  printf("TEST 4 - C\n"); delay(200);
  in_data = 0x000003FF;
  read_data = 0x00;
  shift_data(1, &in_data, &read_data, tms_one); // the last shift insert a TDI 1 and a TMS 1 so that the transition to the next state takes place
  
  delay(4000);

  // current state: EXIT-IR
  // go from EXIT_IR to SHIFT_DR
  send_tms(4, 0b0011);

  delay(4000);

  printf("TEST 4 - D\n"); delay(200);
  // write an abundance of 0 into all (exact number is unknown) BYPASS registers in order to create a defined starting state
  for (int i = 0; i < 9; i++) {
    printf("TEST 4 - E\n"); delay(200);
    in_data = 0x000003FF;
    read_data = 0x00;
    shift_data(10, &in_data, &read_data, tms_zero);
  }
  printf("TEST 4 - F\n"); delay(200);

  delay(4000);

  // while still in SHIFT_DR state, shift in ones and count how many shifts it takes until a one appears on TDO
  int counter = 0;
  bool one_found = false;
  int abort_counter = 0;
  while (!one_found) {

    if (abort_counter > 10) {
      printf("aborting...\n");
      abort_counter = 0;
      break;
    }
    printf("TEST G - A\n"); delay(200);

    printf("TEST H - A\n"); delay(200);
    // shift in a single one, while remaning in the SHIFT_DR state
    in_data = 0x00000001;
    read_data = 0x00;
    shift_data(1, &in_data, &read_data, tms_zero);
    counter++;

    printf("TEST I - A\n"); delay(200);

    if (read_data >= 0x01) {
      one_found = true;
      continue;
    }

    printf("TEST J - A\n"); delay(200);

    delay(1000);

    abort_counter++;
  }
  printf("TEST 4 - K\n"); delay(200);

  printf("device count: %d\n", counter);


  delay(3000);

}
