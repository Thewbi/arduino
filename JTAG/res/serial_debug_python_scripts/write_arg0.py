# This script inserts an data into the dtm.dmi register.
# The dtm.dmi register is used to transfer data from the DTM to the DM.
#
# The transfer requires an address in the DM to write to, a value to write, an two operation bits to specificy the operation (read, write, nop).
# This script makes use of the write operation (10binary).
# The dtm.dmi register is 44 bits wide to accomodate a 10 bit address, a 32 bit data wird / payload and 2 bit for the operation.
# Once the dtm.dmi is written, a wishbone transaction between the JTAG.TAP as wishbone master and the DM as wishbone slave is started.
# The transaction is either a read or write depending on the last two bits that denote the operation (read, write, nop).
#
# This specific script want to write into arg0 which is a DM register having the address 0x04. That is why the address 0x04 is used to
# shift into dtm.dmi.
#
# The ultimate goal why data needs to the shifted into arg0 is that in order to execute abstract commands inside the DM,
# arguments are first written to arg0 and arg1, then an abstract command is written to dm.command (dm register with address 0x17).
# When the DM gets a new abstract command written to dm.command (0x17) it will execute this abstract command.
# The abstract command is to either read or write to memory for example. Other abstract commands are defined.
#
# This script is used to fill in an argument into arg0.


# pip install pyserial

import serial
import time
import serial.tools.list_ports

sleep_duration = 0.3

myports = [tuple(p) for p in list(serial.tools.list_ports.comports())]
print(myports)


ser = serial.Serial('COM4', baudrate=9600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=None)
ser.flushInput()
ser.flushOutput()

# https://forum.arduino.cc/t/arduino-restarts-with-incoming-serial-data-help/21422/2
# The Arduino DUE will reboot whenever a serial connection is opened!
# Give the Arduino some time to boot!
# Then use the connection without closing it if you want the arduino to keep state
# At the end, disconnect.
time.sleep(0.5) # Sleep for the arduino to boot

#packet = bytearray()
#packet.append(0x02)
#packet.append(0x00)
#packet.append(0x03)

#packet = bytearray(b'\x02\x00\x03')

#packet = b"\x02\x00\x03"

#packet = b"\x02"

#packet = b'\x02'

time.sleep(sleep_duration)

# 0 - ping (return is pong (0x50))
#packet = bytearray(b'\x02\x00\x03')
#ser.write(packet)
#     \h(02 00 03)
input = '02 00 03'    
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# 1 - reset to TEST_LOGIC_RESET (wait a couple of seconds, device will return 0x00 (RESULT_OK)
# send_tms(5, 0b11111, 1000);
#     \h(02 01 00 00 00 05 00 00 00 1F 03)
input = '02 01 00 00 00 05 00 00 00 1F 03'    
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# 2 - to SHIFT_IR
# send_tms(5, 0b00110, 1000);
#     \h(02 01 00 00 00 05 00 00 00 06 03)
input = '02 01 00 00 00 05 00 00 00 06 03'    
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# 3 - load SHIFT_IR with IDCODE of the dtm.dmi register (= 0x11)
# in_data = 0x00000011;
# read_data = 0x00;  
# shift_data(31, &in_data, &read_data, tms_zero, 10);
# shift_data(1, &in_data, &read_data, tms_one, 10); // on the last bit, transition to EXIT1_IR by using a tms of 1
#     \h(02 0A 82 00 00 00 1F 00 00 00 11 00 03)
input = '02 0A 82 00 00 00 1F 00 00 00 11 00 03'    
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# 4 -
#     \h(02 0A 82 00 00 00 01 00 00 00 00 01 03)
input = '02 0A 82 00 00 00 01 00 00 00 00 01 03'
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# 5 - capture IR shift into IR data (transition over CAPTURE IR) and finally into SHIFT_DR
# send_tms(6, 0b001110, 1000);
#     \h(02 01 00 00 00 06 00 00 00 0E 03)
input = '02 01 00 00 00 06 00 00 00 0E 03'
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# 6 - write the first 32 of 44 bits into dtm.dmi
#
# [Addr, 10 bit][Data, 32 bit][Operation, 2bit]
# 0x10           0x01          01b (read) == 0x4000000005 == 0x[040][00000005] <--------- READ OPERATION
# 0x10           0x01          10b (write) == 0x4000000006 == 0x[040][00000006] <--------- WRITE OPERATION
# 0x10           0x15          10b (write) == 0x4000000056 == 0x[040][00000056] <--------- WRITE OPERATION
# 0x10           0x80000000    10b (write) == 0x4200000002 == 0x[042][00000002] <--------- WRITE OPERATION
# 0x10           0x80000000    00b (nop) == 0x4200000000 == 0x[042][00000000] <--------- NOP
#
# write dm.data0 register
# 0x04           0x1F1F1F1F    10b (write) == 0x107C7C7C7E == 0x[010][7C7C7C7E]
#
# write dm.data1 register
# 0x05           0x1F1F1F1F    10b (write) == 0x147C7C7C7E == 0x[014][7C7C7C7E]
#
# in_data = 0x00000000;
# read_data = 0x00;
# shift_data(32, &in_data, &read_data, tms_zero, 10);
# [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
#     \h(02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03)
input = '02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03'
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# 7 - write another 11 bits into into dtm.dmi
# in_data = 0x042;
# read_data = 0x00;
# shift_data(11, &in_data, &read_data, tms_zero, 10);
# [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
#     \h(02 0A 82 00 00 00 0B 00 00 00 10 00 03)
input = '02 0A 82 00 00 00 0B 00 00 00 10 00 03'
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# 8 - Write the last bit into dtm.dmi and transition out of that state
#
# last step shifts in data and leaves the state at the same time
# in_data = 0x00;
# read_data = 0x00;
# shift_data(1, &in_data, &read_data, tms_one, 10);
# [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
#     \h(02 0A 82 00 00 00 01 00 00 00 00 01 03)
input = '02 0A 82 00 00 00 01 00 00 00 00 01 03'
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)

time.sleep(sleep_duration)

# TEST: SHIFT DATA OUT AGAIN
#\h(02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03)
#\h(02 0A 82 00 00 00 0B 00 00 00 14 00 03)
#\h(02 0A 82 00 00 00 01 00 00 00 00 00 03)

#\h(02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03)
#\h(02 0A 82 00 00 00 0B 00 00 00 14 00 03)
#\h(02 0A 82 00 00 00 01 00 00 00 00 00 03)

# 9 - enter UPDATE_DR because this triggers the actual write operation towards the wishbone slave
#  printf("Enter UPDATE_DR\n");
#  send_tms(3, 0b000110, 1000);
#     \h(02 01 00 00 00 0A 83 00 00 00 06 03)
input = '02 01 00 00 00 0A 83 00 00 00 06 03'
ser.write(bytes.fromhex(input))
while ser.inWaiting():
    in_hex = ser.read().hex()
    print(in_hex)
    
time.sleep(sleep_duration)
    
ser.close()
    
"""
"""

#while True:
#    in_hex = ser.read().hex()
#    print(in_hex)

#in_hex = ser.read().hex()
#print(in_hex)

#time.sleep(0.5) # Sleep for 3 seconds

#res = ser.read()

#print(res)
#print("my num is 0x{0:02x}".format(res))