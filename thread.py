#!/usr/bin/env python2

import threading
import datetime
import subprocess

def getTime():
    datum = datetime.datetime.now()
    subprocess.call("echo "+datum.strftime('%Y-%m-%d %H:%M:%S')+"> /dev/ds3231_drv", shell = True)
    subprocess.call("cat /dev/ds3231_drv", shell = True)

t1 = threading.Thread(target=getTime)
t2 = threading.Thread(target=getTime)

t1.start()
t2.start()

t1.join()
t2.join()
