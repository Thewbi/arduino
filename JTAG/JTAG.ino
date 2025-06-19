//
// Trouble Shooting:
//
// Close all Arduino IDEs and only have a single one open in order 
// to prevent the IDEs and your terminal emulator to compete for 
// the COM port to the Arduino DUE.
//

//
// Include
//

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//
// PINS
//

const int ledPin = 13; // led pin

//
// OLED i2c Display
//

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//
// JTAG
//

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
const uint8_t STATE_ERROR = 3;

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
void send_tms(size_t len, uint32_t data, uint32_t delay_in_ms);
void shift_data(size_t len, uint32_t* in_data, uint32_t* read_data, uint8_t tms_data, uint32_t delay_in_ms);
void oled_printf_byte(uint8_t data);
void oled_printf(char* data);

// global memory in order to remember what the current jtag clock level is
int8_t jtag_clk_state = -1;

void setup() 
{
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);

  pinMode(jtag_clk, OUTPUT);
  pinMode(jtag_tms, OUTPUT);

  pinMode(jtag_tdi, INPUT);
  pinMode(jtag_tdo, OUTPUT);

  // Set JTAG CLK low
  digitalWrite(jtag_clk, LOW);
  jtag_clk_state = LOW;

  // set TMS low
  digitalWrite(jtag_tms, LOW);

  // set TDO low
  digitalWrite(jtag_tdo, LOW);

  // OLED

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    //Serial.println(F("SSD1306 allocation failed"));
    Serial.println("failed");
    for(;;); // Don't proceed, loop forever
  }

  /*
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds
  */

  oled_printf("Waiting for input.");
  //delay(2000);

/*
  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);
*/

/*
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display().
*/
  //Serial.println("Waiting for input.");
}

void loop() {

  uint32_t in_data = 0x00;
  uint64_t in_data_long = 0x00;
  uint32_t read_data = 0x00;
  uint64_t read_data_long = 0x00;
  uint32_t tms_zero = 0x00;
  uint32_t tms_one = 0x01;  

  // parse message and emit
  if (Serial.available() > 0) {

    // read the incoming byte:
    uint8_t incomingByte = Serial.read();

    // echo
    //Serial.write(incomingByte);

    switch (current_state) {

      case STATE_ERROR:
        break;

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

          oled_printf("[OK]");
        }
        break;

      case STATE_BODY:
        if (incomingByte == STX) {
          // STX moves the state machine back to the STX state and purges the buffer!
          rx_buffer_usage = 0;
          rx_buffer[rx_buffer_usage++] = incomingByte;
          current_state = STATE_STX;

          // inform the client about an error (incomplete message)
          Serial.write(0xFF);
          Serial.write(0xFE);
          Serial.write(0xFD);
          Serial.write(0xFC);

          oled_printf("[ERROR] STX before ETX!");

          // go to error state
          current_state = STATE_ERROR;
        } else if (incomingByte == ETX) {
          // add ETX if possible
          if (rx_buffer_usage == RX_BUFFER_SIZE) {
            
            // inform the client about an error (second STX before ETX)
            Serial.write(0xFF);
            Serial.write(0xFE);
            Serial.write(0xFD);
            Serial.write(0xFC);
            rx_buffer_usage = 0;
            current_state = STATE_IDLE;

            oled_printf("[ERROR] STX before ETX!");

            // go to error state
            current_state = STATE_ERROR;
          } else {
            rx_buffer[rx_buffer_usage++] = incomingByte;
            current_state = STATE_BODY;
          }
          // emit
          rx_buffer_emit = 1;
          rx_buffer_emit_size = rx_buffer_usage;
          
          // reset
          rx_buffer_usage = 0;
          current_state = STATE_IDLE;
        } else {
          // check for buffer overrun, else add character
          if (rx_buffer_usage == RX_BUFFER_SIZE) {
            // inform the client about an error (buffer overrun)
            Serial.write(0xFF);
            Serial.write(0xFE);
            Serial.write(0xFD);
            Serial.write(0xFC);
            rx_buffer_usage = 0;
            current_state = STATE_IDLE;

            oled_printf("[ERROR] Buffer Overrun!");

            // go to error state
            current_state = STATE_ERROR;
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

  char buffer[100];

  // 1. Parse command (0x00 = ping, 0x01 = send_tms, 0x02 = shift_data)
  switch (decode_buffer[0]) {

    case COMMAND_PING:
      oled_printf("[OK] COMMAND_PING");

      // answer with a pong (0x50 ASCII P as in (P)ong), we are alive after all!
      Serial.write(RESPONSE_PING);
      oled_printf("[OK] COMMAND_PING DONE");
      break;

    case COMMAND_SEND_TMS:
      // parameter 0 - number_bits_to_execute
      number_bits_to_execute = decode_buffer[1] << 24 | decode_buffer[2] << 16 | decode_buffer[3] << 8 | decode_buffer[4] << 0;
      // parameter 1 - bits_to_execute
      bits_to_execute = decode_buffer[5] << 24 | decode_buffer[6] << 16 | decode_buffer[7] << 8 | decode_buffer[8] << 0;

      // print feedback onto OLED
      memset(buffer, 0 , 100);
      sprintf((char *)buffer, "COMMAND_SEND_TMS bits: %d bits_to_execute: %d", number_bits_to_execute, bits_to_execute);
      oled_printf(buffer);

      // execute
      send_tms(number_bits_to_execute, bits_to_execute, 100);     

      // answer OK
      Serial.write(RESULT_OK);
      break;

    case COMMAND_SHIFT_DATA:
      //Serial.println("COMMAND_SHIFT_DATA");

      // parameter 0 - in_data - data to shift in (uint32_t)
      number_bits_to_shift = decode_buffer[1] << 24 | decode_buffer[2] << 16 | decode_buffer[3] << 8 | decode_buffer[4] << 0;
      // parameter 1 - in_data - data to shift in (uint32_t)
      in_data = decode_buffer[5] << 24 | decode_buffer[6] << 16 | decode_buffer[7] << 8 | decode_buffer[8] << 0;
      // parameter 2 - bits_to_execute
      tms = decode_buffer[9];

/*
      Serial.print("COMMAND_SHIFT_DATA number_bits_to_shift: ");
      Serial.print(number_bits_to_shift, DEC);
      Serial.print(" in_data: ");
      Serial.print(in_data, DEC);
      Serial.print(" tms: ");
      Serial.println(tms, DEC);
      */

      memset(buffer, 0 , 100);
      sprintf((char *)buffer, "COMMAND_SHIFT_DATA number_bits_to_shift: %d in_data: %d tms: %d", number_bits_to_shift, in_data, tms);

      oled_printf(buffer);

      // execute
      shift_data(number_bits_to_shift, &in_data, &out_data, tms, 100);

      // answer with the shifted in data
      Serial.write((out_data >> 24) & 0xFF);
      Serial.write((out_data >> 16) & 0xFF);
      Serial.write((out_data >> 8) & 0xFF);
      Serial.write((out_data >> 0) & 0xFF);

      break;

    default:
      //Serial.write(0xFC);
      //Serial.write(0xFD);
      //Serial.write(0xFE);
      //Serial.write(0xFF);
      Serial.println("UNKNOWN COMMAND");
      oled_printf("UNKNOWN COMMAND");

      current_state = STATE_ERROR;
      break;

  }
  // 1. Pass bytes to handler for that command
  // 1. Handler parses parameters and executes the command
  // 1. Handler sends response

}

void send_tms(size_t len, uint32_t data, uint32_t delay_in_ms) {
  
  //Serial.print("send_tms len: ");
  //Serial.print(len, DEC);
  //Serial.print(" data: ");
  //Serial.println(data, DEC);

  for (size_t i = 0; i < len; i++) {

    uint8_t bit = data & 0x01;
    data >>= 1;   

    // if the JTAG clock is high, pull it LOW
    // if it is low already, do nothing
    if (jtag_clk_state == HIGH) {
      delay(delay_in_ms);
      digitalWrite(jtag_clk, LOW);
      jtag_clk_state = LOW;
    }

    // set the new value
    digitalWrite(jtag_tms, bit ? HIGH : LOW);

    // wait a little for the signal to stabilize, then pull clock HIGH 
    // in order to trigger the FPGA to transition the JTAG state machine
    delay(delay_in_ms);
    digitalWrite(jtag_clk, HIGH);
    jtag_clk_state = HIGH;
  }
}

void shift_data(size_t len, uint32_t* in_data, uint32_t* read_data, uint8_t tms_data, uint32_t delay_in_ms) 
{
  //Serial.print("shift_data len: ");
  //Serial.print(len, DEC);
  //Serial.print(" in_data: ");
  //Serial.print(((uint32_t)*in_data), DEC);
  //Serial.print(" tms_data: ");
  //Serial.println(tms_data, DEC);

  for (size_t i = 0; i < len; i++) {

    uint8_t bit = *in_data & 0x01;
    *in_data >>= 1;

    delay(delay_in_ms);
    digitalWrite(jtag_clk, LOW);
    jtag_clk_state = LOW;
      
    digitalWrite(jtag_tms, tms_data);
    digitalWrite(jtag_tdo, bit);

    delay(delay_in_ms); 
    digitalWrite(jtag_clk, HIGH);
    jtag_clk_state = HIGH;

    // cause negedge and read data
    delay(delay_in_ms);
    digitalWrite(jtag_clk, LOW);
    jtag_clk_state = LOW;

    delay(delay_in_ms);
    int val = digitalRead(jtag_tdi);
    *read_data >>= 1;
    *read_data |= (val << 7) << 24;
  }
}

// OLED draw byte
void oled_printf_byte(uint8_t data)
{
  char buffer[10]; // Buffer to hold the formatted string
  //int hexValue = 0xA5; // Example hex value
  sprintf(buffer, "0x%X", data); 

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0); // Set cursor position
  display.println(buffer); // Print the formatted string
  display.display(); // Update the display
}

// OLED draw text
void oled_printf(char* data)
{
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  for (int16_t i = 0; i < strlen(data); i++) {
    display.write(data[i]);
  }

  display.display();
}
