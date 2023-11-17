#!/usr/bin/env bash

sudo cp fan_control /opt/fan_control
sudo cp monitor_temp.service monitor_temp.timer
sudo systemctl enable monitor_temp.timer
sudo systemctl start monitor_temp.timer
