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
    delay(500);
    digitalWrite(jtag_clk, HIGH);
    delay(500);
  }
}

void shift_data(size_t len, uint32_t in_data, uint32_t* read_data, uint8_t tms_data) {

  digitalWrite(jtag_clk, LOW);
  delay(500);

  for (size_t i = 0; i < len; i++) { 

    //Serial.println("shift_data");

    uint8_t bit = in_data & 0x01;
    in_data >>= 1;

    
    
    
    digitalWrite(jtag_tms, tms_data);
    digitalWrite(jtag_tdo, bit);    
    digitalWrite(jtag_clk, HIGH);
    delay(500);

    
    digitalWrite(jtag_clk, LOW);
    delay(250);
    int val = digitalRead(jtag_tdi);
    *read_data >>= 1;
    *read_data |= (val << 7) << 24;
    delay(250);

    //Serial.println(*read_data);
    Serial.print(i);
    Serial.print(" - ");
    Serial.println(val);
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
  
  send_tms(5, 0b11111); // reset TEST_LOGIC_RESET
  //send_tms(5, 0b00110); // to SHIFT_IR
  send_tms(4, 0b0010); // to SHIFT_DR

  uint32_t in_data = 0x87654321;
  uint32_t read_data = 0x00;
  shift_data(32, in_data, &read_data, 0);

  Serial.println(read_data, HEX);

}
