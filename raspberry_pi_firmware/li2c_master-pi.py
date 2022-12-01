# Raspberry Pi mMaster fr Arduino Slave
# i2c_master_pi.py
# Conects to Arduino via I2C
import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
from smbus import SMBus
import time

reader = SimpleMFRC522()

text1=["Mecha is on","IITGN Preet"]

bus = SMBus(1)
time.sleep(1)
addr = 0x8 # bus address
numb=1
count=0
def StrToByte(blah):
	converted = []
	for b in blah:
		converted.append(ord(b))
	return converted
	
while numb==1:
	try:
		f=bus.write_i2c_block_data(addr,0x03,StrToByte('D'))
		print(StrToByte('D'))
		print("You there? Check")
		a=bus.read_i2c_block_data(addr,0x03,1)
		if(a==StrToByte('0')):
			print("Hola!")
		print(a)
		time.sleep(0.5)
		#numb=0
		#print("Place Your Tag")
		#id, text = reader.read()
		
		
		#if(text.strip() in text1):
		#	bus.write_byte(addr, 0x0)
		#	print("Authorised User")
		#else:
		count+=1
		if(count==10):
			numb=0
		#	print("UnAuthorised User")
	finally:
		GPIO.cleanup()
	time.sleep(2)
