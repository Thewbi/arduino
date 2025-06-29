# This script executes an abstract command in DM!
# It is called write_dmcommand.py because it writes an abstract command into the DM.command register.
# The abstract command itself can be a read or a write abstract command.
#
# The abstract command (read or write memory) is placed into the DTM.dmi register (= 0x11).
# The command can be a read or a write memory command.
#
# Once placed into the DTM.dmi register (0x11), a wishbone transaction between the JTAG.TAP 
# as wishbone master and the DM as wishbone slave is started.
# 
# The dm.command register (0x17) is filled with an abstract command. In order to do this,
# the DTM.dmi register (0x11) is filled with a write-operation to the address dmi.command (0x17)
# and the payload that defines the abstract command (which can be memory read or write).
#
# Memory read and write abstract commands require parameters / arguments. 
# For example a read expects the memory address to read from in arg1.
# These arguments are not written by this script! (see write_arg0.py, write_arg1.py to write args first before using this script).
#
# When the JTAG TAP / DTM sees this command, it will start a wishbone transaction with it's
# wishbone slave, which is the DM. It will write the abstract command into the DM.command register.
# This immediately triggers the DM to execute the abstract command.
#
# dm.command register (0x17)
#The DM will execute the command and write a value into arg0.


# Test-Script write dm.data1 register

# pip install pyserial



import serial
import time
import serial.tools.list_ports

wait_after_boot = 0.5
response_duration = 0.1
sleep_duration = 0.5

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
    time.sleep(wait_after_boot) # Sleep for the arduino to boot after it is reset by a new serial connection!

    print("0 ----------- PING PONG -----------")
    time.sleep(sleep_duration)


    

    print("1 ----------- 5 Bits ----------")
    time.sleep(sleep_duration)

    #for x in range(3):
    # 1 - reset to TEST_LOGIC_RESET (wait a couple of seconds, device will return 0x00 (RESULT_OK)
    # (5 bit)
    #
    # send_tms(5, 0b11111, 1000);
    #     \h(02 01 00 00 00 05 00 00 00 1F 03)
    data = '02 01 00 00 00 05 00 00 00 1F 03'    
    ser.write(bytes.fromhex(data))

    wait_for_response(ser)

    # UP TO HERE 0 + 5(+1) = 6 bits (first bit is power on glitch, the additional log output is from the tms command first raising the clock which outputs another tick)
    #


    #print("START")
    time.sleep(sleep_duration)

    print("2 ----------- 5 Bits --- (to SHIFT_IR, 0x0B) ----------")
    time.sleep(sleep_duration)
    #name = input("Enter to proceed!\n")

    # 2 - to SHIFT_IR
    # (5 bit)
    #
    # send_tms(5, 0b00110, 1000);
    #     \h(02 01 00 00 00 05 00 00 00 06 03)
    data = '02 01 00 00 00 05 00 00 00 06 03'    
    ser.write(bytes.fromhex(data))

    # read
    wait_for_response(ser)

    # UP TO HERE 5 = 5 bits (first bit is power on glitch)



    ##
    ## Purpose of 3 and 4 is to shift the code for the DMI_INSTRUCTION 32'h00000011 
    ## into the IR register, so that the DTM.dmi register becomes
    ## the current data register and can be filled with data
    ##


    print("3 ----------- 31 Bits --- (remain in SHIFT_IR, 0x0B) ----")
    time.sleep(sleep_duration)
    #name = input("Enter to proceed!\n")

    # 3 - load SHIFT_IR with IDCODE of the DTM.DMI_COMMAND register (= 0x11)
    # (31 bit)
    #
    # in_data = 0x00000011;
    # read_data = 0x00;  
    # shift_data(31, &in_data, &read_data, tms_zero, 10);
    # shift_data(1, &in_data, &read_data, tms_one, 10); // on the last bit, transition to EXIT1_IR by using a tms of 1
    #     \h(02 0A 82 00 00 00 1F 00 00 00 11 00 03)
    data = '02 0A 82 00 00 00 1F 00 00 00 11 00 03'    
    ser.write(bytes.fromhex(data))

    # read
    wait_for_response(ser)

    # UP TO HERE 5 + 31 = 36 bits



    print("4 ----------- 1 Bit ---- (to EXIT1_IR, 0x0C) ---")
    time.sleep(sleep_duration)
    #name = input("Enter to proceed!\n")

    # 4 - send last bit from load SHIFT_IR with IDCODE of the DTM.DMI_COMMAND register (= 0x11) in order to transition to EXIT1_IR
    # (1 BIT)
    #     \h(02 0A 82 00 00 00 01 00 00 00 00 01 03)
    data = '02 0A 82 00 00 00 01 00 00 00 00 01 03'
    ser.write(bytes.fromhex(data))

    # read
    wait_for_response(ser)

    # UP TO HERE 5 + 31 + 1 = 37 bits


    print("5 ----------- 6 Bits ---- (to SHIFT_DR, 0x04) --")
    time.sleep(sleep_duration)
    #name = input("Enter to proceed!\n")

    # 5 - Transition to SHIFT_DR, capture IR shift into IR data (transition over CAPTURE IR) and finally transition into SHIFT_DR
    # send_tms(6, 0b001110, 1000);
    # 6 bits
    #
    #     \h(02 01 00 00 00 06 00 00 00 0E 03)
    data = '02 01 00 00 00 06 00 00 00 0E 03'
    ser.write(bytes.fromhex(data))

    # read
    wait_for_response(ser)

    # UP TO HERE 5 + 31 + 1 + 6 = 43 bits (first bit is power on glitch)




    print("6 ----------- 32 Bits ---- (remain in SHIFT_DR, 0x04) ----")
    time.sleep(sleep_duration)
    #name = input("Enter to proceed!\n")

    # 6 - write the first 32 of 44 bits into DTM.DMI_COMMAND
    # 32 bits
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
    # write dm.command register
    # 0x17           0x02210000    10b (write) == 0x5C08840002 == 0x[05C][08840002]
    #
    # write dm.command register
    # 0x17           0x00000000    10b (write) == 0x5C00000002 == 0x[05C][00000002]
    #
    # in_data = 0x00000000;
    # read_data = 0x00;
    # shift_data(32, &in_data, &read_data, tms_zero, 10);
    # [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
    #     \h(02 0A 82 00 00 00 20 08 84 00 0A 82 00 03)

    ##
    ## Here, the 42 bit dtm.command register is filled.
    ##

    # return value (= read value) is in arg0
    # addr goes into arg1

    ## abstract command
    cmdtype = 0b00000010            # cmdtype (8)           - 0x02 = cmdtype for "access memory"
    aamvirtual = 0b0	            # aamvirtual (1)        - no virtual memory translation
    aamsize = 0b010			        # aamsize (3)           - 2dec means 32 bit data transfer
    aampostincrement = 0b0          # aampostincrement (1)  - no postincrement
    not_used_1 = 0b00  	            # not-used (2)          - not used
    write = 0b1 			        # write (1)             - write = 0, read = 1
    target_specific = 0b00	        # target-specific (2)   - target specific (not used)
    not_used_2 = 0b00000000000000   # not-used (14)         - not used

    abstract_command = 0
    abstract_command = ((abstract_command << 8)  | cmdtype)
    abstract_command = ((abstract_command << 1)  | aamvirtual)
    abstract_command = ((abstract_command << 3)  | aamsize)
    abstract_command = ((abstract_command << 1)  | aampostincrement)
    abstract_command = ((abstract_command << 2)  | not_used_1)
    abstract_command = ((abstract_command << 1)  | write)
    abstract_command = ((abstract_command << 2)  | target_specific)
    abstract_command = ((abstract_command << 14) | not_used_2)

    print("abstract_command is 0x{0:02x}".format(abstract_command))

    command_address = ADDRESS_OF_DM_COMMAND_REGISTER # 0x04 is register data_0
    #command_data = 0x02210000 # value to write into the register
    command_data = abstract_command # value to write into the register
    command_operator = 0b10 # operation to execute, 10b is write

    command_bits = (command_address << 34) | (command_data << 2) | (command_operator << 0);

    #print("command_bits is 0x{0:02x}".format(command_bits))

    

    

    # write abstract command 02210000 "which is read from memory". 
    # address to read from is expected in DM.arg1 (needs to be written by the write_arg1.py script)
    # This abstract command is wrapped inside a JTAG TAP / DTM dmi register write operation in 
    # order to write the wrapped abstract command into DM's command register 17
    #data = '02 0A 82 00 00 00 20 08 84 00 0A 82 00 03'

    # ???
    #data = '02 0A 82 00 00 00 20 00 00 00 0A 82 00 03'
    #ser.write(bytes.fromhex(data))

    command_32bit = command_bits & 0xFFFFFFFF
    command_bits = command_bits >> 32

    amount_of_bits_to_shift = 32
    tms = 0x00
    ser.write(create_shift_data_command(command_32bit, amount_of_bits_to_shift, tms))

    # read
    wait_for_response(ser)




    print("7 ----------- 11 Bits --- (remain in SHIFT_DR, 0x04) ----")
    time.sleep(sleep_duration)
    #name = input("Enter to proceed!\n")

    # 7 - write another 11 bits into into DTM.DMI_COMMAND
    # 11 bits - bitmask 11111111111 = 0x7FF
    # in_data = 0x042;
    # read_data = 0x00;
    # shift_data(11, &in_data, &read_data, tms_zero, 10);
    # [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
    #     \h(02 0A 82 00 00 00 0B 00 00 00 5C 00 03)
    #data = '02 0A 82 00 00 00 0B 00 00 00 5C 00 03'
    #ser.write(bytes.fromhex(data))

    command_11bit = command_bits & 0x7FF
    command_bits = command_bits >> 11
    #print("command_bits is 0x{0:02x}".format(command_10bit))

    amount_of_bits_to_shift = 11
    tms = 0x00
    ser.write(create_shift_data_command(command_11bit, amount_of_bits_to_shift, tms))

    

    # read
    wait_for_response(ser)





    print("8 ----------- 1 Bit --- (to EXIT1-DR, 0x05) ----")
    time.sleep(sleep_duration)

    # 8 - Write the last bit into DTM.DMI_COMMAND and transition out of that state into EXIT1-DR, 0x05
    # (1 Bit)
    #
    # last step shifts in data and leaves the state at the same time
    # in_data = 0x00;
    # read_data = 0x00;
    # shift_data(1, &in_data, &read_data, tms_one, 10);
    # [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
    #     \h(02 0A 82 00 00 00 01 00 00 00 00 01 03)
    #data = '02 0A 82 00 00 00 01 00 00 00 00 01 03'
    #ser.write(bytes.fromhex(data))

    command_1bit = command_bits & 0x01
    command_bits = command_bits >> 1
    #print("command_bits is 0x{0:02x}".format(command_1bit))

    amount_of_bits_to_shift = 1
    tms = 0x01
    ser.write(create_shift_data_command(command_1bit, amount_of_bits_to_shift, tms))


    # read
    wait_for_response(ser)










    print("9 ----------- 3 Bits ---- (to UPDATE_DR, 0x08) ---")
    time.sleep(sleep_duration)
    #name = input("Enter to proceed!\n")

    # TEST: SHIFT DATA OUT AGAIN
    #\h(02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03)
    #\h(02 0A 82 00 00 00 0B 00 00 00 14 00 03)
    #\h(02 0A 82 00 00 00 01 00 00 00 00 00 03)

    #\h(02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03)
    #\h(02 0A 82 00 00 00 0B 00 00 00 14 00 03)
    #\h(02 0A 82 00 00 00 01 00 00 00 00 00 03)

    # 9 - enter UPDATE_DR because this triggers the actual write operation towards the wishbone slave
    # 3 Bits
    #  printf("Enter UPDATE_DR\n");
    #  send_tms(3, 0b000110, 1000);
    #     \h(02 01 00 00 00 0A 83 00 00 00 06 03)
    data = '02 01 00 00 00 0A 83 00 00 00 06 03'    
    ser.write(bytes.fromhex(data))

    # read
    wait_for_response(ser)




    # terminate connection
    time.sleep(sleep_duration)  
    ser.close()



if __name__ == "__main__":
    main()