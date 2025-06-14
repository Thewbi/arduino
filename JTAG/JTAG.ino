//
// PINS
//

const int ledPin = 13; // led pin

const int jtag_clk = 22; // JTAG_CLK
const int jtag_tms = 24; // JTAG_TMS

const int jtag_tdi = 26; // JTAG_TDI
const int jtag_tdo = 28; // JTAG_TDO

//
// INTERFACE
//

const uint8_t STX = 0x02;
const uint8_t ETX = 0x03;
const uint8_t ESCAPE_MARKER = 0x0A;

const uint8_t STATE_IDLE = 0;
const uint8_t STATE_STX = 1;
const uint8_t STATE_BODY = 2;

uint8_t current_state = STATE_IDLE;

const uint8_t RX_BUFFER_SIZE = 32;
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint8_t rx_buffer_usage = 0;
uint8_t rx_buffer_emit = 0;
uint8_t rx_buffer_emit_size = 0;

const uint8_t COMMAND_PING = 0x00;
const uint8_t RESPONSE_PING = 0x50;
const uint8_t COMMAND_SEND_TMS = 0x01;
const uint8_t COMMAND_SHIFT_DATA = 0x02;
const uint8_t RESULT_OK = 0x00;

void emit();

void send_tms(size_t len, uint32_t data, uint32_t delay_in_ms) {
  
  Serial.print("send_tms len: ");
  Serial.print(len, DEC);
  Serial.print(" data: ");
  Serial.println(data, DEC);

  for (size_t i = 0; i < len; i++) {    
    uint8_t bit = data & 0x01;
    data >>= 1;
    digitalWrite(jtag_clk, LOW);
    digitalWrite(jtag_tms, bit ? HIGH : LOW);
    digitalWrite(jtag_clk, HIGH);

    delay(delay_in_ms);
  }
}

void shift_data(size_t len, uint32_t* in_data, uint32_t* read_data, uint8_t tms_data, uint32_t delay_in_ms) {

  Serial.print("shift_data len: ");
  Serial.print(len, DEC);
  Serial.print(" in_data: ");
  Serial.print(((uint32_t)*in_data), DEC);
  Serial.print(" tms_data: ");
  Serial.println(tms_data, DEC);

  digitalWrite(jtag_clk, LOW);
  for (size_t i = 0; i < len; i++) {
    uint8_t bit = *in_data & 0x01;
    *in_data >>= 1;      
    digitalWrite(jtag_tms, tms_data);
    digitalWrite(jtag_tdo, bit);    
    digitalWrite(jtag_clk, HIGH);
    digitalWrite(jtag_clk, LOW);
    int val = digitalRead(jtag_tdi);
    *read_data >>= 1;
    *read_data |= (val << 7) << 24;

    delay(delay_in_ms);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);

  pinMode(jtag_clk, OUTPUT);
  pinMode(jtag_tms, OUTPUT);

  pinMode(jtag_tdi, INPUT);
  pinMode(jtag_tdo, OUTPUT);

  printf("start");
}

void loop() {

  uint32_t in_data = 0x00;
  uint64_t in_data_long = 0x00;
  uint32_t read_data = 0x00;
  uint64_t read_data_long = 0x00;
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
  
  /*
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
  
  // Start to load 1s into all instruction registers of all devices 
  // in the entire JTAG chain because an IR register filled with
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
  */

  /*
  // Test 5 - read and write the dmi register (0x11)

  // reset to TEST_LOGIC_RESET
  printf("To TEST_LOGIC_RESET\n");
  send_tms(5, 0b11111, 1000);

  // to SHIFT_IR
  printf("To SHIFT_IR\n");
  send_tms(5, 0b00110, 1000);

  // load SHIFT_IR with IDCODE of the dmi register
  printf("Load IR with dmi register instruction\n");
  in_data = 0x00000011;
  read_data = 0x00;  
  shift_data(31, &in_data, &read_data, tms_zero, 10);
  shift_data(1, &in_data, &read_data, tms_one, 10); // on the last bit, transition to EXIT1_IR by using a tms of 1
  
  // capture IR and shift into IR data (transition over CAPTURE IR) and enter SHIFT_DR
  printf("Enter SHIFT_DR\n");
  send_tms(6, 0b001110, 1000);

  // current state: SHIFT_DR. STEP: shift in 44 bits and stay in SHIFT_DR
  in_data = 0x01234567;
  read_data = 0x00;
  shift_data(32, &in_data, &read_data, tms_zero, 10);
  in_data = 0x89A;
  read_data = 0x00;
  shift_data(12, &in_data, &read_data, tms_zero, 10);

  // current state: SHIFT_DR. STEP: shift in 44 bits and stay in SHIFT_DR
  in_data = 0x00000000;
  read_data = 0x00;
  shift_data(32, &in_data, &read_data, tms_zero, 10);
  Serial.println("LO:");
  Serial.println(read_data, HEX);
  in_data = 0x89A;
  read_data = 0x00;
  shift_data(12, &in_data, &read_data, tms_zero, 10);
  Serial.println("HI:");
  Serial.println(read_data, HEX);
  */
  
  /*
  // Test 6 - read and write the dmi register (0x11)

  // reset to TEST_LOGIC_RESET
  printf("To TEST_LOGIC_RESET\n");
  send_tms(5, 0b11111, 1000);

  // to SHIFT_IR
  printf("To SHIFT_IR\n");
  send_tms(5, 0b00110, 1000);

  // load SHIFT_IR with IDCODE of the dmi register
  printf("Load IR with dmi register instruction\n");
  in_data = 0x00000011;
  read_data = 0x00;  
  shift_data(31, &in_data, &read_data, tms_zero, 10);
  shift_data(1, &in_data, &read_data, tms_one, 10); // on the last bit, transition to EXIT1_IR by using a tms of 1
  
  // capture IR and shift into IR data (transition over CAPTURE IR) and enter SHIFT_DR
  printf("Enter SHIFT_DR\n");
  send_tms(6, 0b001110, 1000);

  // current state: SHIFT_DR. STEP: shift in 44 bits and stay in SHIFT_DR
  //
  // [Addr, 10 bit][Data, 32 bit][Operation, 2bit]
  // 0x00           0x00          01 (write)
  //
  // HI
  in_data = 0x00000001;
  read_data = 0x00;
  shift_data(32, &in_data, &read_data, tms_zero, 10);
  in_data = 0x00;
  read_data = 0x00;
  shift_data(11, &in_data, &read_data, tms_zero, 10);
  // last step shifts in data and leaves the state at the same time
  in_data = 0x00;
  read_data = 0x00;
  shift_data(1, &in_data, &read_data, tms_one, 10);

  // enter UPDATE_DR because this triggers the actual write operation towards the wishbone slave
  printf("Enter UPDATE_DR\n");
  send_tms(3, 0b000110, 1000);
  */

  /*
  // Test 7 - read and write the dtm.dmi_register (0x11) in order to write to the dm.dm_control register (0x10)

  printf("TEST 7\n");

  // reset to TEST_LOGIC_RESET
  printf("To TEST_LOGIC_RESET\n");
  send_tms(5, 0b11111, 10);

  // to SHIFT_IR
  printf("To SHIFT_IR\n");
  send_tms(5, 0b00110, 10);

  // load SHIFT_IR with IDCODE of the dmi register (= 0x11)
  printf("Load IR with dmi register instruction\n");
  in_data = 0x00000011;
  read_data = 0x00;  
  shift_data(31, &in_data, &read_data, tms_zero, 10);
  shift_data(1, &in_data, &read_data, tms_one, 10); // on the last bit, transition to EXIT1_IR by using a tms of 1
  
  // capture IR and shift into IR data (transition over CAPTURE IR) and enter SHIFT_DR
  printf("Enter SHIFT_DR\n");
  send_tms(6, 0b001110, 10);

  // current state: SHIFT_DR. STEP: shift in 44 bits and stay in SHIFT_DR
  //
  // [Addr, 10 bit][Data, 32 bit][Operation, 2bit]
  // 0x10           0x01          01 (read) == 0x4000000005 == 0x[040][00000005] <--------- READ OPERATION
  // 0x10           0x01          10 (write) == 0x4000000006 == 0x[040][00000006] <--------- WRITE OPERATION
  // 0x10           0x15          10 (write) == 0x4000000056 == 0x[040][00000056] <--------- WRITE OPERATION
  // 0x10           0x80000000    10 (write) == 0x4200000002 == 0x[042][00000002] <--------- WRITE OPERATION
  //
  // LO
  in_data = 0x00000002;
  read_data = 0x00;
  shift_data(32, &in_data, &read_data, tms_zero, 10);

  //
  // HI
  in_data = 0x042;
  read_data = 0x00;
  shift_data(11, &in_data, &read_data, tms_zero, 10);

  // last step shifts in data and leaves the state at the same time
  //in_data = 0x00;
  //read_data = 0x00;
  shift_data(1, &in_data, &read_data, tms_one, 10);

  // enter UPDATE_DR because this triggers the actual write operation towards the wishbone slave
  printf("Enter UPDATE_DR\n");
  send_tms(3, 0b000110, 10);

  delay(3000);
  */

/**/
  int state = STX;

  // parse message and emit
  if (Serial.available() > 0) {

    // read the incoming byte:
    uint8_t incomingByte = Serial.read();

    // echo
    //Serial.write(incomingByte);

    switch (current_state) {

      case STATE_IDLE:
        if (incomingByte == STX) {
          rx_buffer[rx_buffer_usage++] = incomingByte;
          current_state = STATE_STX;
        }
        break;

      case STATE_STX:
        if (incomingByte == STX) {
          // nop
        } else if (incomingByte == ETX) {
          rx_buffer_usage = 0;
          current_state = STATE_IDLE;
        } else {
          rx_buffer[rx_buffer_usage++] = incomingByte;
          current_state = STATE_BODY;
        }
        break;

      case STATE_BODY:
        if (incomingByte == STX) {
          rx_buffer_usage = 0;
          rx_buffer[rx_buffer_usage++] = incomingByte;
          current_state = STATE_STX;
        } else if (incomingByte == ETX) {
          // add ETX if possible
          if (rx_buffer_usage == RX_BUFFER_SIZE) {
            Serial.write(0xFF);
            Serial.write(0xFE);
            Serial.write(0xFD);
            Serial.write(0xFC);
            rx_buffer_usage = 0;
            current_state = STATE_IDLE;
          } else {
            rx_buffer[rx_buffer_usage++] = incomingByte;
            current_state = STATE_BODY;
          }
          // emit
          rx_buffer_emit = 1;
          rx_buffer_emit_size = rx_buffer_usage;
          //// emit
          //for (int i = 0; i < rx_buffer_usage; i++) {
          //  Serial.write(rx_buffer[i]);
          //}
          // reset
          rx_buffer_usage = 0;
          current_state = STATE_IDLE;
        } else {
          if (rx_buffer_usage == RX_BUFFER_SIZE) {
            Serial.write(0xFF);
            Serial.write(0xFE);
            Serial.write(0xFD);
            Serial.write(0xFC);
            rx_buffer_usage = 0;
            current_state = STATE_IDLE;
          } else {
            rx_buffer[rx_buffer_usage++] = incomingByte;
            current_state = STATE_BODY;
          }
        }
        break;

    }

    if (rx_buffer_emit) {      

      emit();

      // reset emit data
      rx_buffer_emit = 0;
      rx_buffer_emit_size = 0;
    }

    // say what you got:
    //Serial.print("I received: ");
    //Serial.println(incomingByte, HEX);
  }  

//delay(100);

}

void emit() {

  //// DEBUG - encoded
  //for (int i = 0; i < rx_buffer_emit_size; i++) {
  //  Serial.write(rx_buffer[i]);        
  //}

  int decode_buffer_usage = 0;
  int decode_buffer[RX_BUFFER_SIZE];
  int escape_active = 0;

  // 1. Remove STX, ETX for an individually received message from the stream
  // 1. Decode: 0x0A 0x82 => 0x02, 0x0A 0x83 => 03, 0x0A 0x8A => 0x0A
  for (int i = 0; i < rx_buffer_emit_size; i++) {

    switch (rx_buffer[i]) {

      case ESCAPE_MARKER:
        escape_active = 1;
        break;

      case 0x82:
        if (escape_active) {
          escape_active = 0;
          decode_buffer[decode_buffer_usage++] = 0x02;
        } else {
          decode_buffer[decode_buffer_usage++] = rx_buffer[i];
        }
        break;

      case 0x83:
        if (escape_active) {
          escape_active = 0;
          decode_buffer[decode_buffer_usage++] = 0x03;
        } else {
          decode_buffer[decode_buffer_usage++] = rx_buffer[i];
        }
        break;

      case 0x8A:
        if (escape_active) {
          escape_active = 0;
          decode_buffer[decode_buffer_usage++] = 0x0A;
        } else {
          decode_buffer[decode_buffer_usage++] = rx_buffer[i];
        }
        break;

      case STX:
      case ETX:
        continue;

      default:
        decode_buffer[decode_buffer_usage++] = rx_buffer[i];
        break;

    }
  }

  //// DEBUG - output decoded
  //for (int i = 0; i < decode_buffer_usage; i++) {
  //  Serial.write(decode_buffer[i]);        
  //}

  uint32_t number_bits_to_execute = 0;
  uint32_t bits_to_execute = 0;

  size_t number_bits_to_shift = 0;
  uint32_t in_data = 0;
  uint8_t tms = 0;

  uint32_t out_data = 0;

  // 1. Parse command (0x00 = ping, 0x01 = send_tms, 0x02 = shift_data)
  switch (decode_buffer[0]) {

    case COMMAND_PING:
      Serial.println("COMMAND_PING");

      // answer with a pong (0x50 ASCII P as in (P)ong), we are alive after all!
      //Serial.write(RESPONSE_PING);
      break;

    case COMMAND_SEND_TMS:
      

      // parameter 0 - number_bits_to_execute
      number_bits_to_execute = decode_buffer[1] << 24 | decode_buffer[2] << 16 | decode_buffer[3] << 8 | decode_buffer[4] << 0;
      // parameter 1 - bits_to_execute
      bits_to_execute = decode_buffer[5] << 24 | decode_buffer[6] << 16 | decode_buffer[7] << 8 | decode_buffer[8] << 0;

      Serial.print("COMMAND_SEND_TMS bits: ");
      Serial.print(number_bits_to_execute, DEC);
      Serial.print(" bits_to_execute: ");
      Serial.println(bits_to_execute, DEC);

      // execute
      send_tms(number_bits_to_execute, bits_to_execute, 10);

      

      // answer OK
      //Serial.write(RESULT_OK);
      break;

    case COMMAND_SHIFT_DATA:
      //Serial.println("COMMAND_SHIFT_DATA");

      // parameter 0 - in_data - data to shift in (uint32_t)
      number_bits_to_shift = decode_buffer[1] << 24 | decode_buffer[2] << 16 | decode_buffer[3] << 8 | decode_buffer[4] << 0;
      // parameter 1 - in_data - data to shift in (uint32_t)
      in_data = decode_buffer[5] << 24 | decode_buffer[6] << 16 | decode_buffer[7] << 8 | decode_buffer[8] << 0;
      // parameter 2 - bits_to_execute
      tms = decode_buffer[9];

      Serial.print("COMMAND_SHIFT_DATA number_bits_to_shift: ");
      Serial.print(number_bits_to_shift, DEC);
      Serial.print(" in_data: ");
      Serial.print(in_data, DEC);
      Serial.print(" tms: ");
      Serial.println(tms, DEC);

      // execute
      shift_data(number_bits_to_shift, &in_data, &out_data, tms, 10);

      //Serial.write((out_data >> 24) & 0xFF);
      //Serial.write((out_data >> 16) & 0xFF);
      //Serial.write((out_data >> 8) & 0xFF);
      //Serial.write((out_data >> 0) & 0xFF);

      break;

    default:
      //Serial.write(0xFC);
      //Serial.write(0xFD);
      //Serial.write(0xFE);
      //Serial.write(0xFF);
      Serial.println("UNKNOWN");
      break;

  }
  // 1. Pass bytes to handler for that command
  // 1. Handler parses parameters and executes the command
  // 1. Handler sends response

}

