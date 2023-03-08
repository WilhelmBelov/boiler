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
#variable to write of data (druck, temperature...) from arduino
data = ""

#mainly cycles
while True:
    tic = time.perf_counter()
    try:
        Data = q.get()

        if Data[0:6]=="serNum":
            Data=int(Data[6:len(Data)])#index of Data message
            print(mes[Data])
            messageFile = open("Message_report.txt")
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
            elif Data==14 or Data==21 or Data==38 or Data==31 or Data==57 or Data==54:
                value = q.get()
                mesData = "{0} {1} {2}\n".format(datetime.datetime.now(), mes[Data], value)
            #arduino message
            else:
                mesData = "{0} {1}\n".format(datetime.datetime.now(), mes[Data])
            
            #file to write of data (druck, temperature...) from arduino
            if data != "":
                #create a new file for write al to first line
                dataFile = open("copie.txt", "a")
                dataFile.write(data)
                #overwriting old file
                with open("Data_report.txt") as f:
                    for line in f:
                        dataFile.write(line)
                    f.close()
                #file size limit 100Mb
                dataFile.flush()
                file_stats = os.stat("copie.txt")
                #print(file_stats.st_size)
                if file_stats.st_size>104857600:
                    dataFile.truncate(104857600)
                dataFile.close()
                #remove old file
                os.remove("Data_report.txt")
                #rename new file
                os.rename("copie.txt", "Data_report.txt")
                data=""
            #file to write of message from arduino
            if mesData != "":
                #create a new file for write al to first line
                dataFile = open("copie.txt", "a")
                dataFile.write(mesData)
                #overwriting old file
                toc = time.perf_counter()
                print(f"First part do in {toc - tic:0.4f} seconds")
                tic = time.perf_counter()
                with open("Message_report.txt") as f:
                    for line in f:
                        dataFile.write(line)
                    f.close()
                toc = time.perf_counter()
                print(f"Second part do in {toc - tic:0.4f} seconds")
                tic = time.perf_counter()
                #file size limit 100Mb
                dataFile.flush()
                file_stats = os.stat("copie.txt")
                print("file size:{0}".format(file_stats.st_size))
                if file_stats.st_size>104857600:
                    dataFile.truncate(104857600)
                dataFile.close()
                #remove old file
                os.remove("Message_report.txt")
                #rename new file
                os.rename("copie.txt", "Message_report.txt")
                mesData=""
        else:
            print("Data is not serial nummer data.") 

    except IndexError as err:
        print(err)
        print("Data is not serial nummer data.")  
    except KeyboardInterrupt:
        print("Closed.")
        decryptFile.close()
        messageFile.close()
        dataFile.close()
        sys.exit()
    toc = time.perf_counter()
    print(f"Part part do in {toc - tic:0.4f} seconds")