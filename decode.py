#!/usr/bin/env python3
import sys
import __future__

HEADER_BOOT =  0x00
HEADER_UPDATE = 0x01
HEADER_BUTTON_CLICK = 0x02
HEADER_BUTTON_HOLD  = 0x03

header_lut = {
    HEADER_BOOT: 'BOOT',
    HEADER_UPDATE: 'UPDATE',
    HEADER_BUTTON_CLICK: 'BUTTON_CLICK',
    HEADER_BUTTON_HOLD: 'BUTTON_HOLD'
}

def decode(data):
    if len(data) < 12:
        raise Exception("Bad data length, min 12 characters expected")

    header = int(data[0:2], 16)
    temperature = int(data[4:6], 16) if data[2:4] != 'ffff' else None
    soil = int(data[6:8], 16) if data[2:4] != 'ffff' else None

    temperature /= 10.0
    soil /= 10.0


    resp = {
        "header": header_lut[header],
        "voltage": int(data[2:4], 16) / 10.0 if data[2:4] != 'ff' else None,
        "temperature": temperature,
        "soil": soil

    }

    return resp


def pprint(data):
    print('Header :', data['header'])
    print('Voltage :', data['voltage'])
    print('Temperature :', data['temperature'])
    print('Soil Moisture :', data['soil'])



if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] in ('help', '-h', '--help'):
        print("usage: python3 decode.py [data]")
        print("example: python3 decode.py 012000e500e7")
        exit(1)

    data = decode(sys.argv[1])
    pprint(data)
