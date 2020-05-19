#!/usr/bin/python -
# -*- coding: utf-8 -*-
#
# Example program to receive packets from the radio link
#
# Also Echoes the packet back to the sender, in the same way as the
# Arduino ping-pong sketches.
#
import os
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
from lib_nrf24 import NRF24
import csv
import time
import spidev
from datetime import datetime

pipes = [
  [0x52, 0x56, 0x43, 0x45, 0x52],
  [0x53, 0x4e, 0x41, 0x52, 0x54]
]

radio = NRF24(GPIO, spidev.SpiDev())
radio.begin(0, 17)

radio.setRetries(15,15)

radio.setPayloadSize(32)
radio.setChannel(0x60)
radio.setDataRate(NRF24.BR_1MBPS)
radio.setPALevel(NRF24.PA_MAX)

radio.setAutoAck(True)
radio.enableDynamicPayloads()
radio.enableAckPayload()

radio.openWritingPipe(pipes[0])
radio.openReadingPipe(1, pipes[1])

radio.startListening()
radio.stopListening()

radio.printDetails()

radio.startListening()

while True:
    pipe = [1]
    while not radio.available(pipe):
        time.sleep(10000/1000000.0)

    recv_buffer = []
    radio.read(recv_buffer, radio.getDynamicPayloadSize())
    print(f'Received Buffer: {recv_buffer}')

    # The radio can only send bytes (integer from 0 to 255)
    # So we must translate the receivedMessage into unicode characters...
    string = ''
    for n in recv_buffer:
        if n == 0:                           # null terminator indicates end of string
            break
        if (n >= 32 and n <= 126):           # Decode into ASCII
            string += chr(n)
    print(f'Received: {string}')

    # The received string should be a comma-delimited list
    data = string.split(',')
    print('data:', data)

    timestamp = datetime.now()
    fields = [timestamp.isoformat()] + data
    print('Fields:', ','.join([str(field) for field in fields]))

    # Echo the packet number (first item in list)
    radio.writeAckPayload(1, [int(data[1])], 1)

    outfile = '/opt/wireless/data/{0}-data.csv'.format(timestamp.strftime('%Y-%m-%d'))
    mode = 'a' if os.path.exists(outfile) else 'w+'
    with open(outfile, mode) as f:
        writer = csv.writer(f)
        writer.writerow([str(field) for field in fields])
