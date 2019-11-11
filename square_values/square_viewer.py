#!/usr/bin/env python

# Recieves a list of comma-seperated values from the serial port, renders it as a square image

import serial
import math
from serial.tools import list_ports
import pprint
import pygame

def map(x, in_min, in_max, out_min, out_max):
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

pygame.init()

#Find the serial ports
ports = list_ports.grep("ttyACM")
serial_port = None

try:
    #Get the first port, or fail if there are none
    port = ports.next()
    serial_port = serial.Serial()
    serial_port.baudrate = 115200
    serial_port.port = port.device
    serial_port.timeout = 30
    serial_port.open()
    if not serial_port.is_open:
        print "Could not open serial port {0}".format(port.device)

except StopIteration:
    print "No serial ports found"

if serial_port is None:
	exit()

screen_w = screen_h = 400
screen = pygame.display.set_mode((screen_w, screen_h))
 
running = True


seen_min = 0
seen_max = 0

while running:
	#Handle pygame events
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

	line = serial_port.readline()

	# Debug, generates a line
	#line = ", ".join([str(x) for x in range(0, 256, 4)])

	try:
		values = [float(x) for x in line.split(',')]
	except ValueError:
		continue

	# Make values square
	side_len = int(math.floor(math.sqrt(len(values))))
	values = values[:int(side_len * side_len)]

	print(values)

	seen_max = max(seen_max, max(values))
	seen_min = min(seen_min, min(values))

	#Convert values to red/blue
	#Assumes max value is 1000, min val is 0
	colors = [(int(map(x, seen_min, seen_max, 0, 255)), 0, int(255 - map(x, seen_min, seen_max, 0, 255))) for x in values]

	#Pixels per value
	ppv = screen_w/side_len

	for x in range(side_len):
		for y in range(side_len):
			color = colors[x + (y * side_len)]
			pygame.draw.rect(screen, color, pygame.Rect((x*ppv, y*ppv),(ppv, ppv)))
	pygame.display.update()