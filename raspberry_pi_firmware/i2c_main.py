import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import sys
from smbus import SMBus
import time
from gsheet import update_sheet

# Usefule Variables
items_dict = {}
items_dict['A'] = ""
items_dict['B'] = "Wheel"
items_dict['C'] = ""
items_dict['D'] = "Solder Wire"
items_dict['E'] = "Cutter"
items_dict['F'] = "L-Brackets"

no_of_compartments = [5]
cupboard_addresses = [0x8, 0x4]
reader=SimpleMFRC522()
valid_IDs=["Mecha is on","IITGN Preet"]

def StrToByte(blah):
	converted = []
	for b in blah:
		converted.append(ord(b))
	return converted

def main(args):
	print()
	print("___WELCOME TO ALPHA DISPENSER___")
	I2Cbus=SMBus(1)
	#with SMBus(1) as I2Cbus:
	slaveSelect= int(input("Which CupBoard (1 or 2): "))
	if slaveSelect < len(cupboard_addresses):
		slaveAddress = cupboard_addresses[slaveSelect - 1]
	else:
		print("No Cupboard selected")
		return None
	
	### Initiation and cupboard connectivity
	ByteSend=StrToByte('0')
	# print("You there? Check")
	# print(ByteSend)
	flag = 1
	while (flag == 1):
		try:
			I2Cbus.write_i2c_block_data(slaveAddress, 0x02, ByteSend)
			flag = 0
		except:
			time.sleep(1)
	#I2Cbus.write_i2c_block_data(slaveAddress, 0x02, ByteSend)
	time.sleep(0.5
	
	try:
		data=I2Cbus.read_i2c_block_data(slaveAddress,0x02,1)
	except:
		print("Unable to connect to the cupboard.")
		time.sleep(0.5)
		return None
	# print(data)
	if (data == StrToByte('0')):
		pass
	else:
		print("There are some errror with the cupboard " + str(slaveSelect))
		return None
		
		# print("Yo there!")
		# print("Place Your Tag")
		
		### User Identification
	print("Scan your ID Card")
	id, text = reader.read()
	if(text.strip() in valid_IDs):
		print("Authorised User")
	else:
		print("UnAuthorised User")
		return None
	
		### Bracing up the cupboard
	print("Unloocking the cupboard")
	I2Cbus.write_i2c_block_data(slaveAddress, 0x02, StrToByte('A'))  #solenoid opening
	response_byte = StrToByte('1')
	
	while (response_byte != StrToByte('0')):
		try:
			response_byte=I2Cbus.read_i2c_block_data(slaveAddress,0x02,1)
			print(response_byte)
			print("In here")
			time.sleep(4)
		except:
			print("Unable to unlock, trying again")
			time.sleep(0.5)
			
	time.sleep(1)
		#check for double error for solenoid response
	wait1 = input("Enter 0 after issuing and closing the door: ")
		
		### Bracing up the cupboard
	if wait1 == "0":
		pass
	else:
		print("The user has entered an invalid value, terminate the dispensing")
		return None
			
		### Relaxing the cupboard
	flag = 1
	while (flag == 1):
		try:
			I2Cbus.write_i2c_block_data(slaveAddress, 0x02, StrToByte('C')) #solenoid closing
			flag = 0
		except:
			time.sleep(1)
			
	response_byte3 = StrToByte('2')
	
	while (response_byte3 != StrToByte('0')):
		try:
			response_byte3=I2Cbus.read_i2c_block_data(slaveAddress,0x02,1)
			time.sleep(0.5)
		except:
			print("Unable to retrive, trying again")
			time.sleep(0.5)	
	
	#response_byte3=I2Cbus.read_i2c_block_data(slaveAddress,0x02,1)
	#print(response_byte3)
			#time.sleep(0.5)
	time.sleep(1)
				
		#check for double error for solenoid closing
		#print("Relax now! The dispensing is over")
		
		### Read and update the change
	flag = 1
	while (flag == 1):
		try:
			I2Cbus.write_i2c_block_data(slaveAddress, 0x02, StrToByte('E')) #solenoid closing
			flag = 0
		except:
			time.sleep(1)
	response_byte2 = StrToByte('1')
	
	while (response_byte2 != StrToByte('0')):
		try:
			response_byte2=I2Cbus.read_i2c_block_data(slaveAddress,0x02,1)
			print(response_byte2)
			print("In here")
			time.sleep(4)
		except:
			print("Unable to lock, trying again")
			time.sleep(0.5)	
	
	flag = 1
	while (flag == 1):
		try:
			I2Cbus.write_i2c_block_data(slaveAddress, 0x02, StrToByte('B')) #solenoid closing
			flag = 0
		except:
			time.sleep(1)
			
	time.sleep(4)
	flag = 1
	while(flag == 1):
		try:
			change=I2Cbus.read_i2c_block_data(slaveAddress, 0x02,15)
			flag = 0
		except:
			time.sleep(1)
	
	time.sleep(2)
	print(change)
	user = text.strip()
		# change = change[1:len(change)-1]
		# change = [int(i) for i in change.split(",")]
	#Identify the change
	change = "".join([chr(i) for i in change]).strip()
	print(change)
	for i in range(0, len(change), 3):
		compartment = item_dict[change[i]]
		value_change = int(change[i+1:i+3]) - 50
		if (value_change != 0):
			update_sheet([user, compartment, value_change])			
		
				
	
if __name__=='__main__':
	while True:
		main(sys.argv)
		
		
