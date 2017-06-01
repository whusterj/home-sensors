# -*- coding: utf-8 -*-
"""
Services to support the Sensors Flask app.
"""
import os
import csv
from datetime import datetime, timedelta


DATA_DIR = '/opt/wireless/data'


def _get_recent_file_names():
    """Get the file names for today and yesterday's CSVs"""
    file_name_tpl = '{}-data.csv'
    date_fmt = '%Y-%m-%d'
    now = datetime.now()
    one_day_ago = now - timedelta(days=1)
    file_names = [
        file_name_tpl.format(one_day_ago.strftime(date_fmt)),
        file_name_tpl.format(now.strftime(date_fmt)),
    ]
    return [os.path.join(DATA_DIR, x) for x in file_names]


def make_recent_data_csv(file_names=None, out_file='/opt/wireless/data/recent-data.csv'):
    """Combine multiple CSV files and filter to data w/in the past day.

    NOTE:
        The order of the `file_names` matters!
    """
    headers = [
        'timestamp',
        'packet_num',
        'source_addr',
        'light_val',
        'soil_val',
        'battery',
    ]
    if file_names is None:
        file_names = _get_recent_file_names()

    print 'Merging data from files: ', file_names

    # Get all rows from all files
    all_rows = []
    for file_name in file_names:
        if os.path.exists(file_name):
            with open(file_name, 'r') as f:
                reader = csv.reader(f)
                for row in reader:
                    all_rows.append(row)

    # Discard rows older than 1 day
    # Note ISO timestamps can be compared as strings, which is quite fast!
    one_day_ago = datetime.now() - timedelta(days=1)
    filtered_rows = [row for row in all_rows if row[0] > one_day_ago.isoformat()]

    # Then write them all to the outfile with headers
    with open(out_file, 'w+') as f:
        writer = csv.writer(f)
        writer.writerow(headers)
        for row in filtered_rows:
            writer.writerow(row)

