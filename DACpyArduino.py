import serial
import time
import types
import re
# First connect the Arduino with the PC and make sure that the correct program is uploaded to it.
# Check the arduino port from the Arduino app -> Tools -> Port: /dev/........
arduino = serial.Serial('/dev/cu.usbmodem14301',57600)
time.sleep(1)

LSB = 19e-6 # This is the DAC resolution in V. To be defined with a more accurate value for a better calibration.
VrefN = -9.998406 # negative reference voltage in V +/- 4uV
VrefP = 10.001550 # positive reference voltage in V +/- 6uV

def setout(outputcode): #It does not work well because everytime it performs the RESET of the Arduino.
	DACin = 524288 # DACcode for 0V output. 
	if( type(outputcode)==type('string') and (outputcode[-1]=='V' or outputcode[-1]=='v') ):
		voutput = int(outputcode[:-1])
		DACin = (voutput-VrefN)/LSB
		DACin = int(DACin)
	elif (type(outputcode)==type(1) or (type(outputcode)==type('string') and int(outputcode)==int(re.search(r'\d+', outputcode).group())) ):
		DACin = int(outputcode)
	if DACin<0 or DACin>1048575:
		print('Error! - Give a non negative integer value in the range 0 (-10V) and 1048575 (+10V).')
	else:
		print('Setting output at',DACin,'DACcode.')
		command = str(DACin)
		arduino.write(str.encode(command)) #str.encode('str') # converts string to byte form.

	


def scan(start=0, stop=1048575, step=40960, delay=100):
	if(not(isinstance(start,int) and isinstance(stop,int) and isinstance(step,int) and isinstance(delay,int) ) ):
		print('Error! - Not valid input types.')
		return
	if(start<0 or start>1048575 or stop<0 or stop>1048575 or step<-1048575 or step>1048575 or delay<0 or delay>60000):
		print('Error! - Not valid input values.')
		return
	# start, stop, step are DAC codes, delay in ms
	command='SCAN#'+str(start)+'#'+str(stop)+'#'+str(step)+'#'+str(delay)+'#'
	#print(command)
	#print(type(command))
	steps=(stop-start)/step
	Vstart=start*20/(2**20) -10
	Vstop=stop*20/(2**20) -10
	arduino.write(str.encode(command)) #str.encode('str') # converts string to byte form.
	print('- Scan from ',start,'(%2fV)'%Vstart, ' to ', stop,'(%2fV)'%Vstop, ' with a step of ', step, 'and delay ',delay, ' ... (do not interrupt)')
	time.sleep((delay/1000)*int(steps))
	print('Completed!')

def reset():
	print('- Performing RESET..'),
	arduino.write(str.encode('RESET'))
	time.sleep(1)
	print('Completed!')
	
#scan()
scan(300000,400000,10000,500)
#reset()
#setout(490000)

