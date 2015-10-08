import Adafruit_BBIO.GPIO as GPIO
import time;
GPIO.setup("P8_16", GPIO.OUT)
GPIO.output("P8_16", GPIO.HIGH)
time.sleep(1)
