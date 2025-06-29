# pip install pyserial

import serial
import time
import serial.tools.list_ports

wait_after_boot = 0.5
response_duration = 0.05
sleep_duration = 0.1

def wait_for_response(ser):
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
    print(" ".join("{:02x}".format(b) for b in in_hex))
    
    

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

    #packet = bytearray()
    #packet.append(0x02)
    #packet.append(0x00)
    #packet.append(0x03)

    #packet = bytearray(b'\x02\x00\x03')

    #packet = b"\x02\x00\x03"

    #packet = b"\x02"

    #packet = b'\x02'

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

    # 3 - load SHIFT_IR with IDCODE of the dmi register (= 0x11)
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

    wait_for_response(ser)





    time.sleep(sleep_duration)

    # 5 - capture IR shift into IR data (transition over CAPTURE IR) and finally into SHIFT_DR
    # send_tms(6, 0b001110, 1000);
    #     \h(02 01 00 00 00 06 00 00 00 0E 03)
    input = '02 01 00 00 00 06 00 00 00 0E 03'
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)





    time.sleep(sleep_duration)

    # 6 - write the first 32 of 44 bits into DTM.DMI_COMMAND
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
    # read dm.data0 register
    # 0x04           0x00000000    01b (read) == 0x1000000001 == 0x[010][000000001]
    #
    # in_data = 0x00000000;
    # read_data = 0x00;
    # shift_data(32, &in_data, &read_data, tms_zero, 10);
    # [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
    #     \h(02 0A 82 00 00 00 20 00 00 00 01 00 03)
    input = '02 0A 82 00 00 00 20 00 00 00 01 00 03'
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)





    time.sleep(sleep_duration)

    # 7 - write another 11 bits into into DTM.DMI_COMMAND
    # in_data = 0x042;
    # read_data = 0x00;
    # shift_data(11, &in_data, &read_data, tms_zero, 10);
    # [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
    #     \h(02 0A 82 00 00 00 0B 00 00 00 10 00 03)
    input = '02 0A 82 00 00 00 0B 00 00 00 10 00 03'
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)





    time.sleep(sleep_duration)

    # 8 - Write the last bit into DTM.DMI_COMMAND and transition out of that state
    #
    # last step shifts in data and leaves the state at the same time
    # in_data = 0x00;
    # read_data = 0x00;
    # shift_data(1, &in_data, &read_data, tms_one, 10);
    # [STX] [CMD] [NUMBER_OF_BITS_TO_SHIFT] [BITS_TO_SHIFT] [TMS_VALUE] [ETX]
    #     \h(02 0A 82 00 00 00 01 00 00 00 00 01 03)
    input = '02 0A 82 00 00 00 01 00 00 00 00 01 03'
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)





    time.sleep(sleep_duration)

    # 9 - enter UPDATE_DR because this triggers the actual write operation towards the wishbone slave
    #  printf("Enter UPDATE_DR\n");
    #  send_tms(3, 0b000110, 1000);
    #     \h(02 01 00 00 00 0A 83 00 00 00 06 03)
    input = '02 01 00 00 00 0A 83 00 00 00 06 03'
    ser.write(bytes.fromhex(input))

    wait_for_response(ser)

    # TEST: SHIFT DATA OUT AGAIN
    #\h(02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03)
    #\h(02 0A 82 00 00 00 0B 00 00 00 14 00 03)
    #\h(02 0A 82 00 00 00 01 00 00 00 00 00 03)

    #\h(02 0A 82 00 00 00 20 7C 7C 7C 7E 00 03)
    #\h(02 0A 82 00 00 00 0B 00 00 00 14 00 03)
    #\h(02 0A 82 00 00 00 01 00 00 00 00 00 03)





    ##
    ## After a small wait, shift out 32bit of the dmi register to 
    ## read the result of the abstract command triggered by the write
    ##

    #print("10 ---------- 3 Bits ---- (from UPDATE_DR (0x08) to SHIFT_DR (0x04) ---")
    time.sleep(2)

    # 2 - to SHIFT_DR
    # (5 bit)
    #
    # send_tms(3, 0b001, 1000);
    #     \h(02 01 00 00 00 05 00 00 00 06 03)
    data = '02 01 00 00 00 0A 83 00 00 00 01 03'    
    ser.write(bytes.fromhex(data))

    wait_for_response(ser)





    #print("11 ---------- 32 Bits ---- (Remain in SHIFT_DR (0x04) and shift out 32 bits) ---")
    data = '02 0A 82 00 00 00 20 00 00 00 00 00 03'

    # ???
    #data = '02 0A 82 00 00 00 20 00 00 00 0A 82 00 03'

    ser.write(bytes.fromhex(data))

    wait_for_response(ser)





    # terminate the connection
    time.sleep(sleep_duration)    
    ser.close()

if __name__ == "__main__":
    main()