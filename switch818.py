import Adafruit_BBIO.GPIO as GPIO
import time;
GPIO.setup("P8_18", GPIO.IN)
switch_state = GPIO.input("P8_18")
if switch_state == 1 :
	f = open('switch818_state.dat', 'w')
	f.write('1\n')
	f.close
else :
	f = open('switch818_state.dat', 'w')
	f.write('0\n')
	f.close

