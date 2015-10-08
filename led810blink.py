import Adafruit_BBIO.GPIO as GPIO
import time;
GPIO.setup("P8_10", GPIO.OUT)
GPIO.output("P8_10", GPIO.HIGH)
time.sleep(1)
