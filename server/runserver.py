#!/usr/bin/python
# -*- coding: utf-8 -*-
from sensors import sensors

if __name__ == '__main__':
    sensors.sio.run(sensors.app)

