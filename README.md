# Wireless Sensor Network Project

This is a sandbox for various projects related to setting up a wireless sensor network of mainly battery-powered arduino nodes reporting back to a Raspberry Pi as their gateway to the net.

The RPi records data from the nodes and provides a web interface to view node data and interact with active nodes (switches, etc.).


## Raspberry Pi Scripts & Service

The Raspberry Pi is the central controller of our sensor network, so it should have an always-running process listening for input and dispatching commands to sensor nodes.

As of this writing (May 27, 2017), I have built a simple listener ('receive.py') that accepts sensor data and appends it to a CSV file with a timestamp. The 'sensor-receiver.service' configuration for systemd can be used to auto-start and manage this script.


## Installation

The Python code and configuration files are designed to run on debian-based Linux (Raspbian, Ubuntu, etc.), so bear that in mind!

Clone the repository to your home server (raspberry pi or what-have-you). The following commands assume you cloned it to your home (`~`) directory.

Create a symlink from `/opt` to the repository directory.

    sudo ln -s ~/home-sensors /opt/home-sensors

Install the nginx configuration:

    sudo ln -s ~/home-sensors/nginx/sensors.conf /etc/nginx/sites-available/
    sudo ln -s /etc/nginx/sites-available/sesnors.conf /etc/nginx/sites-enabled/

Install the systemd scripts:

    sudo cp ~/home-sensors/server/systemd/sensor-dashboard.service /lib/systemd/system/
    sudo cp ~/home-sensors/server/systemd/sensor-receiver.service /lib/systemd/system/
    sudo systemctl enable sensor-dashboard sensor-receiver
    sudo systemctl start sensor-dashboard sensor-receiver
