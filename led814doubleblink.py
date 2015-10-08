import Adafruit_BBIO.GPIO as GPIO
import time;
GPIO.setup("P8_14", GPIO.OUT)
GPIO.output("P8_14", GPIO.HIGH)
time.sleep(0.15)
GPIO.output("P8_14", GPIO.LOW)
time.sleep(0.15)
GPIO.output("P8_14", GPIO.HIGH)
time.sleep(0.15)
