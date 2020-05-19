#!/usr/bin/python -
# -*- coding: utf-8 -

#
# Monkey-patching the standard library with eventlet is required
# for the redis back-end to work.
#
import eventlet
eventlet.monkey_patch()

import os
import time
import dateutil

from flask import Flask, render_template, send_from_directory
import pyinotify
from flask_socketio import SocketIO, emit, disconnect

from .services import make_recent_data_csv


# Set to 'threading', 'eventlet', or 'gevent'
async_mode = 'eventlet'

app = Flask(__name__, static_url_path='')
sio = SocketIO(app, message_queue='redis://', async_mode=async_mode)
thread = None


@app.route('/', methods=['GET'])
def index():
    return render_template('sensor-dashboard.html')


@sio.on('connect')
def ws_connect():
    print('Client connected')
    emit('connection_confirmed', {'data': 'Connected', 'count': 0})
    global thread
    if thread is None:
        thread = sio.start_background_task(target=bg_file_watch_thread)


@sio.on('in_message')
def ws_in_message(message):
    print('message ', data)


@sio.on('disconnect')
def ws_disconnect():
    print('Client disconnected')


def bg_file_watch_thread():
    print('Started background file watch thread')
    # Get the latest data file
    data_dir = '/opt/wireless/data/'
    last_file_size = None
    latest_file = None
    latest_file_date = None

    while True:
        print('Checking for file updates')
        for file in os.listdir(data_dir):
            if file.startswith('.'):
                continue
            parts = file.split('-')
            if len(parts) > 3:
                date = [int(x) for x in parts[:3]]
                if latest_file_date is None or date > latest_file_date:
                    latest_file_date = date
                    latest_file = file

        print('Getting info for file: {}'.format(latest_file))

        try:
            new_size = os.stat(os.path.join(data_dir, latest_file)).st_size
        except OSError:
            # The file was removed or something
            new_size = None

        print('comparing {} with {}'.format(new_size, last_file_size))
        if new_size != last_file_size:
            print('Making recent-data.csv')
            make_recent_data_csv()
            print('Emitting file_updated event')
            sio.emit('file_updated')
            last_file_size = new_size

        sio.sleep(5)


#if __name__ == '__main__':
#    sio.run(app, debug=True)

