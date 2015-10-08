import Adafruit_BBIO.GPIO as GPIO
import time;
GPIO.setup("P8_14", GPIO.OUT)
GPIO.output("P8_14", GPIO.HIGH)
time.sleep(1)
