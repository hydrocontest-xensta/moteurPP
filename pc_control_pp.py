#!/usr/bin/python3
# coding: utf-8
import serial
import time

from typing import List

PORT = '/dev/ttyUSB0'
MOTOR_NODE = 1
POLYNOMIAL = 0xD5


SER = serial.Serial(PORT, baudrate=115200, bytesize=serial.EIGHTBITS,
                    parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1)
SOF = ord('S')
EOF = ord('E')
assert SOF == 0x53
assert EOF == 0x45

def int_from_bytes(data:bytes, signed=True):
    unsigned_val =  int.from_bytes(data, byteorder='little')
    if signed:
    # sign is the first bit of first byte
        sign_bit = data[-1] & 0x80
        bit_width = len(data)*8
        if sign_bit:
            signed_val = unsigned_val - (1 << bit_width)
            return signed_val 
    return unsigned_val

assert int_from_bytes(bytes([0xff]), signed=True) == -1
assert int_from_bytes(bytes([0xff, 0xff]), signed=True) == -1
assert int_from_bytes(bytes([0x21]), signed=True) == 33
assert int_from_bytes(bytes([0xff]), signed=False) == 255


def frame_str(frame: bytes, sep=" ", delim="`"):
    str_frame = [hex(byte) for byte in frame]
    out = sep.join(str_frame)
    return "_{}_".format(out)


def CalcCRCByte(u8Byte: int, u8CRC: int):
    u8CRC = u8CRC ^ u8Byte
    for _ in range(8):
        if (u8CRC & 0x01):
            u8CRC = (u8CRC >> 1) ^ POLYNOMIAL
        else:
            u8CRC >>= 1
    return u8CRC


def calc_crc(data: List[int]):
    crc = 0xFF
    for byte in data:
        crc = CalcCRCByte(byte, crc)
    return crc


def build_command(command: int, data: List[int])->None:
    data_size = len(data)
    user_data_length = data_size + 4
    telegram_length = user_data_length + 2
    tg = [0] * telegram_length
    tg[0] = SOF
    tg[1] = user_data_length
    tg[2] = MOTOR_NODE
    tg[3] = command
    tg[4:data_size+4] = data
    tg[user_data_length] = calc_crc(tg[1:user_data_length])
    tg[user_data_length + 1] = EOF
    # print('net', tg)
    frame = bytes(tg)
    # print('built', frame_str(frame))
    assert is_valid_frame(frame)
    return frame


def build_sdo_read(index_hb: int, index_lb: int, subindex: int):
    command = 0x01
    data = [index_lb, index_hb, subindex]
    return build_command(command, data)


def send_telegram(telegram: bytes):
    SER.write(telegram)
    print("sent telegram: {}".format(frame_str(telegram)))

def send_soft_reset():
    telegram = build_command(command = 0x00, data = [])
    send_telegram(telegram)
    ok, payload = receive_telegram()
    if ok:
        mn, udl, command, data, crc = payload
        drive_name = data.decode('ascii')
        print("got boot-up from : '{}'".format(drive_name))

def read_param(index_hb: int, index_lb: int, subindex: int, signed_value = True):
    telegram = build_sdo_read(index_hb, index_lb, subindex)
    SER.write(telegram)
    success,  payload = receive_telegram()
    if not success:
        print("read param failed")
        return False, None

    mn, userdatalength, command, data, crc = payload
    # data_size = userdatalength - 4
    # parse data
    obj = data[0:3]
    value = data[3:]
    # print("read param")
    # print("node number", mn)
    # print("udl", userdatalength)

    # print("{} [{}]-> {}".format(command, data_size,  frame_str(data)))
    # LOW - HIGH - SUB
    human_value = int_from_bytes(value, signed=signed_value)
    print("obj {:02x}{:02x}.{:x} has value {}\t({})".format(obj[1], obj[0], obj[2], human_value, frame_str(value)))
    # print("crc = ", crc)

    return True, human_value


def get_pos():
    read_param(0x60, 0x64, 0)


def receive_telegram():
    raw_response = SER.read_until(terminator='E', size=64)
    # print(frame_str(raw_response), raw_response)
    if len(raw_response) == 0:
        print("receive telegram got nothing")
        return False, (None, )
    # scan to first SOF
    try:
        sof_index = raw_response.index(SOF)
    except ValueError:
        print("no SOF found")
        return False, (None, )
    raw_response = raw_response[sof_index:]

    if not is_valid_frame(raw_response):
        print("receive got invalid telegram")
        return False, (None, )
    user_data_length = raw_response[1]
    motor_node = raw_response[2]
    command = raw_response[3]
    data = raw_response[4:user_data_length]
    crc = raw_response[user_data_length]
    return True, (motor_node, user_data_length, command, data, crc)


def send_numbers():
    loop_ix = 0
    while True:
        msg = "{}t".format(loop_ix % 9)
        raw_msg = msg.encode()
        SER.write(raw_msg)
        print("sent [{}]".format(msg))
        loop_ix += 1
        raw_response = SER.read_until(terminator='E', size=64)
        print(raw_response.decode('ascii'))
        time.sleep(1)


def is_valid_frame(frame: bytes):
    if(frame[0] != SOF) or (frame[-1] != EOF):
        return False
    
    user_data_length = frame[1]
    if user_data_length+2 != len(frame):
        return False
    
    motor_node = frame[2]
    if motor_node != 1:
        return False
    
    # CRC test
    their_crc = frame[user_data_length]
    our_crc = calc_crc(frame[1:user_data_length])
    if their_crc != our_crc:
        return False

    return True



def test_frames():
    """
    test againts reference frames to check algo
    """
    status_tx = bytes([0x53, 0x07, 0x01, 0x01, 0x41, 0x60, 0x00, 0x73, 0x45])
    status_response = bytes(
        [0x53, 0x09, 0x01, 0x01, 0x41, 0x60, 0x00, 0x40, 0x04, 0x6C, 0x45])
    pos_tx = bytes([0x53 ,0x07 ,0x01 ,0x01 ,0x64 ,0x60 ,0x00 ,0x56 ,0x45])
    pos_res = bytes([0x53 ,0x0B ,0x01 ,0x01 ,0x64 ,0x60 ,0x00 ,0x09 ,0x00 ,0x00 ,0x00 ,0x53 ,0x45])
    # assert is_valid_frame(status_tx)
    # assert is_valid_frame(status_response)
    assert is_valid_frame(pos_tx)
    # assert is_valid_frame(pos_res)


def main():
    send_soft_reset()
    while True:
        get_pos()
        time.sleep(.5)
        # break


if __name__ == "__main__":
    test_frames()
    main()
# if __name__ == "__main__":
#     send_numbers()
