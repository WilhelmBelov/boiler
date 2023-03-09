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
#variable to write of message from arduino
mesData = ""
fNameMes = "Message_report.txt"
#variable to write of data (druck, temperature...) from arduino
data = ""
fNameData = "Data_report.txt"
count=0

#mainly cycles
while True:
    #tic = time.perf_counter()
    try:
        Data = q.get()
        #protection overloading
        if(q.qsize()>15):
            q.queue.clear()
            
        if Data[0:6]=="serNum":
            Data=int(Data[6:len(Data)])#index of Data message
            print(mes[Data])
            #druck - start parametr
            if Data==58:
                value = q.get()
                data = "\n{0} {1}:{2}".format(datetime.datetime.now(), mes[Data], value)
            #parametrs: average druck, temperature, deltaHeat, deltaWait
            elif Data==59 or Data==60 or Data==50 or Data==47:
                value = q.get()
                data = " {0}:{1}".format(mes[Data], value)
            #parametr tensArr - convert to kWatt
            elif Data==51:
                value = q.get()
                power=0
                for bit in value:
                    if bit==1:
                        power=power+4
                data = " {0}:{1}".format(mes[Data], power)
            #commentare to parametrs
            elif Data==46 or Data==45 or Data==49 or Data==48:
                data = " <<{0}>>".format(mes[Data])
            #arduino message with value
            elif Data==14 or Data==21 or Data==38 or Data==31 or Data==57 or Data==54  or Data==62:
                value = q.get()
                mesData = "{0} {1} {2}\n".format(datetime.datetime.now(), mes[Data], value)
            #arduino message
            else:
                mesData = "{0} {1}\n".format(datetime.datetime.now(), mes[Data])
            
            #file to write of data (druck, temperature...) from arduino
            if data != "":
                #create a new file for write al to first line
                dataFile = open(fNameData, "a")
                dataFile.write(data)
                dataFile.close()
                file_stats = os.stat(fNameData)
                #print(file_stats.st_size)
                if file_stats.st_size>1048576:
                    fNameData="{0}{1}.txt".format(fNameData[0:11], count)
                    count=count+1
                data=""
            #file to write of message from arduino
            if mesData != "":
                #create a new file for write al to first line
                dataFile = open(fNameMes, "a")
                dataFile.write(data)
                dataFile.close()
                file_stats = os.stat(fNameMes)
                #print(file_stats.st_size)
                if file_stats.st_size>1048576:
                    fNameMes="{0}{1}.txt".format(fNameMes[0:14], count)
                    count=count+1
                mesData=""
        else:
            print("Data is not serial nummer data.") 

    except IndexError as err:
        print(err)
        print("Data is not serial nummer data.")  
    except KeyboardInterrupt:
        print("Closed.")
        decryptFile.close()
        sys.exit()
    #toc = time.perf_counter()
    #print(f"Part part do in {toc - tic:0.4f} seconds")