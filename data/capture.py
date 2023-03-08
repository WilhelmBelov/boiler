#!/usr/bin/env python3

import serial
import datetime
import time
import queue
import threading
import sys
import os

q = queue.Queue()

#connection with arduino through UART interface (usb)
def arduino_worker():
    print("Init serial.")
    #you can to find devise with command ls /dev/tty* 
    #name devise ‘/ dev / ttyACM0’, ‘/ dev / ttyUSB0’
    serStatusOf=True
    usb=0
    while True: 
        try:
            if serStatusOf:
                device="/dev/ttyUSB{0}".format(usb)
                ser = serial.Serial(device, 115200, timeout=5000)
                ser.flush()
                serStatusOf=False
                print("Looking for available data...")
            else:
                if ser.in_waiting > 0:
                    #print("Read available data: ")
                    line = ser.readline()
                    line = line.decode('utf-8').rstrip()
                    print(line)
                    #send arduino-data to queue
                    q.put(line)
        except serial.serialutil.SerialException:
            print("Error serial connection.")
            usb=usb+1 #look all and find port with device
            if usb==7:
                usb=0
            time.sleep(1)
        except OSError:
            print("Serial was unconnected.")
            serStatusOf=True
        except KeyboardInterrupt:
            break
        
# Turn-on the arduino_worker thread.
threading.Thread(target=arduino_worker, daemon=True).start()

#handling arduino data

#file to decrypt of message from arduino
decryptFile = open(os.path.join(sys.path[0], "Serial_message.txt"))
mes = decryptFile.read().split('\n')
#print(mes)
#file to write of message from arduino
messageFile = open("Message_report.txt", "a")
#file to write of data (druck, temperature...) from arduino
dataFile = open("Data_report.txt", "a")

#mainly cycles
while True:
    try:
        Data = q.get()
        
        if Data[0:6]=="serNum":
            Data=int(Data[6:len(Data)])#index of Data message
            #druck - start parametr
            if Data==58:
                value = q.get()
                print("Write to file:")
                print("\n{0} {1}:{2}".format(datetime.datetime.now(), mes[Data], value))
                dataFile.write("\n{0} {1}:{2}".format(datetime.datetime.now(), mes[Data], value))
            #parametrs: average druck, temperature, deltaHeat, deltaWait
            elif Data==59 or Data==60 or Data==50 or Data==47:
                value = q.get()
                print("Write to file:")
                print(" {0}:{1}".format(mes[Data], value))
                dataFile.write(" {0}:{1}".format(mes[Data], value))
            #parametr tensArr - convert to kWatt
            elif Data==51:
                value = q.get()
                power=0
                for bit in value:
                    if bit==1:
                        power=power+4
                print("Write to file:")
                print(" {0}:{1}".format(mes[Data], power))
                dataFile.write("2023-03-03 21:57:09.793453 LCD write error: 0 TEST")
            #commentare to parametrs
            elif Data==46 or Data==45 or Data==49 or Data==48:
                print("Write to file:")
                print(" <<{0}>>".format(mes[Data]))
                dataFile.write("2023-03-03 21:57:09.793453 LCD write error: 0 TEST")
            #arduino message with value
            elif Data==14 or Data==21 or Data==38 or Data==31 or Data==57 or Data==54:
                value = q.get()
                print("Write to file:")
                print("{0} {1} {2}\n".format(datetime.datetime.now(), mes[Data], value))
                messageFile.write("2023-03-03 21:57:09.793453 LCD write error: 0 TEST")
            #arduino message
            else:
                print("Write to file:")
                print("{0} {1}\n".format(datetime.datetime.now(), mes[Data]))
                messageFile.write("2023-03-03 21:57:09.793453 LCD write error: 0 TEST")
        else:
            print("Data is not serial nummer data.") 

        #file size limit 100Mb
        dataFile.truncate(1000)
        messageFile.truncate(1000)
    except IndexError as err:
        print(err)
        print("Data is not serial nummer data.")  
    except KeyboardInterrupt:
        print("Closed.")
        decryptFile.close()
        messageFile.close()
        dataFile.close()
        sys.exit()