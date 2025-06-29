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

wait_after_boot = 0.5
response_duration = 0.05
sleep_duration = 0.1

COMMAND_SHIFT_DATA = 0x02

ADDRESS_OF_DTM_DMI_REGISTER = 0x11

ADDRESS_OF_DM_DATA_0_REGISTER = 0x04
ADDRESS_OF_DM_DATA_1_REGISTER = 0x05
ADDRESS_OF_DM_COMMAND_REGISTER = 0x17

def wait_for_response(ser):
    # read
    #print("a")
    byte_count = 0
    in_hex = bytearray()
    response_received = 0
    while response_received == 0:
        while ser.inWaiting():
            byte_count += ser.inWaiting()
            #print("received: ", byte_count)
            xx = ser.read()
            in_hex.extend(xx)
            response_received = 1
        time.sleep(response_duration)
    #print("b")
    #print(in_hex)
    #print("received: ", byte_count)
    

# This function constructs a SHIFT_DATA command (0x02) 
def create_shift_data_command(payload, amount_of_bits_to_shift, tms):
    
    command_32bit_bytearray = bytearray()
    #command_32bit_bytearray.extend(b'\x02') # STX
    command_32bit_bytearray.extend(COMMAND_SHIFT_DATA.to_bytes(1, 'big')) # COMMAND (0x02 = shift data)
    command_32bit_bytearray.extend(amount_of_bits_to_shift.to_bytes(4, 'big')) # Bits to shift
    command_32bit_bytearray.extend(payload.to_bytes(4, 'big')) # data to shift
    command_32bit_bytearray.extend(tms.to_bytes(1, 'big')) # TMS Value
    #command_32bit_bytearray.extend(b'\x03') # ETX

    # print before transfer encoding
    #print(" ".join("{:02x}".format(b) for b in command_32bit_bytearray))

    # apply transfer bytes
    command_32bit_bytearray = command_32bit_bytearray.replace(b"\x0A", b"\x0A\x8A").replace(b"\x02", b"\x0A\x82").replace(b"\x03", b"\x0A\x83")

    # print after transfer encoding
    #print(" ".join("{:02x}".format(b) for b in command_32bit_bytearray))

    # warp in STX, ETX
    transfer_bytearray = bytearray()
    transfer_bytearray.extend(b'\x02')
    transfer_bytearray.extend(command_32bit_bytearray)
    transfer_bytearray.extend(b'\x03')

    #print(" ".join("{:02x}".format(b) for b in transfer_bytearray))
    
    return transfer_bytearray
    
def main():

    #myports = [tuple(p) for p in list(serial.tools.list_ports.comports())]
    #print(myports)

    ser = serial.Serial('COM4', baudrate=9600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=None)
    ser.flushInput()
    ser.flushOutput()

    # https://forum.arduino.cc/t/arduino-restarts-with-incoming-serial-data-help/21422/2
    # The Arduino DUE will reboot whenever a serial connection is opened!
    # Give the Arduino some time to boot!
    # Then use the connection without closing it if you want the arduino to keep state
    # At the end, disconnect.
    time.sleep(wait_after_boot) # Sleep for the arduino to boot


    time.sleep(sleep_duration)

    # 0 - ping (return is pong (0x50))
    #packet = bytearray(b'\x02\x00\x03')
    #ser.write(packet)
    #     \h(02 00 03)
    input = '02 00 03'    
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)
    

    time.sleep(sleep_duration)

    # 1 - reset to TEST_LOGIC_RESET (wait a couple of seconds, device will return 0x00 (RESULT_OK)
    # send_tms(5, 0b11111, 1000);
    #     \h(02 01 00 00 00 05 00 00 00 1F 03)
    input = '02 01 00 00 00 05 00 00 00 1F 03'    
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)
    

    time.sleep(sleep_duration)

    # 2 - to SHIFT_IR
    # send_tms(5, 0b00110, 1000);
    #     \h(02 01 00 00 00 05 00 00 00 06 03)
    input = '02 01 00 00 00 05 00 00 00 06 03'    
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)

    time.sleep(sleep_duration)

    # 3 - load SHIFT_IR with IDCODE of the dtm.dmi register (= 0x11)
    # in_data = 0x00000011;
    # read_data = 0x00;  
    # shift_data(31, &in_data, &read_data, tms_zero, 10);
    # shift_data(1, &in_data, &read_data, tms_one, 10); // on the last bit, transition to EXIT1_IR by using a tms of 1
    #     \h(02 0A 82 00 00 00 1F 00 00 00 11 00 03)
    input = '02 0A 82 00 00 00 1F 00 00 00 11 00 03'    
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)

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

    wait_for_response(ser)





    ##
    ## Here, the 42 bit dtm.command register is filled.
    ##

    # Test:
    # 1. Write address 0x000004 into data1
    # 2. Write data 0xCAFEBABE or 0xDEADBEEF into data0
    # 3. Execute abstract command "write"
    # 4. Write data 0x00000000 into data0
    # 5. Execute abstract command "read"
    # 6. Read data0
    # Result is: 0xCAFEBABE or 0xDEADBEEF is in data0

    command_address = ADDRESS_OF_DM_DATA_0_REGISTER # 0x04 is register data_0
    command_data = 0x00000000
    #command_data = 0xCAFEBABE # value to write into the register
    #command_data = 0xDEADBEEF
    command_operator = 0b10 # operation to execute, 10b is write

    command_bits = (command_address << 34) | (command_data << 2) | (command_operator << 0);

    print("command_bits is 0x{0:02x}".format(command_bits))

    

    

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
    #input = '02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03'
    #ser.write(bytes.fromhex(input))

    # cut the value into 32 bit, 11 bit and 1 bit!

    command_32bit = command_bits & 0xFFFFFFFF
    command_bits = command_bits >> 32
    #print("command_bits is 0x{0:02x}".format(command_32bit))
    
    amount_of_bits_to_shift = 32
    tms = 0x00
    ser.write(create_shift_data_command(command_32bit, amount_of_bits_to_shift, tms))

    wait_for_response(ser)
    
    
    
    
    
    
    
    
    
    
    
    time.sleep(sleep_duration)

    command_11bit = command_bits & 0x7FF
    command_bits = command_bits >> 11
    print("command_bits is 0x{0:02x}".format(command_11bit))

    # 7 - write another 11 bits into into dtm.dmi
    # in_data = 0x042;
    # read_data = 0x00;
    # shift_data(11, &in_data, &read_data, tms_zero, 10);
    # [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
    #     \h(02 0A 82 00 00 00 0B 00 00 00 10 00 03)
    #input = '02 0A 82 00 00 00 0B 00 00 00 10 00 03'
    #ser.write(bytes.fromhex(input))
    
    amount_of_bits_to_shift = 11
    tms = 0x00
    ser.write(create_shift_data_command(command_11bit, amount_of_bits_to_shift, tms))

    wait_for_response(ser)
    
    
    
    
    
    
    # write last bit and transition out of SHIFT_DR at the same time
    time.sleep(sleep_duration)
    
    command_1bit = command_bits & 0x01
    command_bits = command_bits >> 1
    print("command_bits is 0x{0:02x}".format(command_1bit))








    time.sleep(sleep_duration)

    # 8 - Write the last bit into dtm.dmi and transition out of that state
    #
    # last step shifts in data and leaves the state at the same time
    # in_data = 0x00;
    # read_data = 0x00;
    # shift_data(1, &in_data, &read_data, tms_one, 10);
    # [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
    #     \h(02 0A 82 00 00 00 01 00 00 00 00 01 03)
    #input = '02 0A 82 00 00 00 01 00 00 00 00 01 03'
    #ser.write(bytes.fromhex(input))
    
    amount_of_bits_to_shift = 1
    tms = 0x01
    ser.write(create_shift_data_command(command_1bit, amount_of_bits_to_shift, tms))

    wait_for_response(ser)










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

    wait_for_response(ser)
        
    # terminate the connection
    time.sleep(sleep_duration)        
    ser.close()
    
if __name__ == "__main__":
    main()