#!/usr/bin/env bash
gcc main.c -o fan_control -lpigpio -O3 -march=native -std=c89
mv fan_control $HOME
