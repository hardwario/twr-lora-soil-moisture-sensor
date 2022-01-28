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
    if len(data) < 16:
        raise Exception("Bad data length, min 12 characters expected")

    header = int(data[0:2], 16)
    soil_temperature = int(data[4:8], 16) if data[4:8] != 'ffff' else None
    core_temperature = int(data[12:16], 16) if data[12:16] != 'ffff' else None

    soil_temperature /= 10.0
    core_temperature /= 10.0

    resp = {
        "header": header_lut[header],
        "voltage": int(data[2:4], 16) / 10.0 if data[2:4] != 'ff' else None,
        "soil_temperature": soil_temperature,
        "soil_moisture": int(data[8:12], 16) if data[8:12] != 'ffff' else None,
        "core_temperature": core_temperature
    }

    return resp

def pprint(data):
    print('Header :', data['header'])
    print('Voltage :', data['voltage'])
    print('Soil temperature :', data['soil_temperature'])
    print('Soil Moisture :', data['soil_moisture'])
    print('Core temperature :', data['core_temperature'])


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] in ('help', '-h', '--help'):
        print("usage: python3 decode.py [data]")
        print("example: python3 decode.py 012000e500e7")
        exit(1)

    data = decode(sys.argv[1])
    pprint(data)
