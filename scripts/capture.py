# capture.py -- capture Arduino (or any) serial output and save it to a text file.
#
# usage:
#   python capture.py COM4 mylog.txt
#       capture COM4 at 115200 baud, save to mylog.txt, stop at "=== DONE ==="
#
#   python capture.py COM4 mylog.txt DONE
#       same, but stop when a line contains "DONE" (custom stop word)
#
#   python capture.py COM4 mylog.txt DONE 9600
#       also set a custom baud rate (default 115200)
#
#   python capture.py COM4 mylog.txt NONE
#       no stop marker -- run until you press Ctrl+C
#
# args: <port> <outfile> [stop_word] [baud]
#   port      = COM port, e.g. COM4  (Arduino IDE -> Tools -> Port)
#   outfile   = whatever you want the file named, e.g. test1.txt
#   stop_word = line containing this ends the capture (default "DONE";
#               use NONE to disable and stop with Ctrl+C)
#   baud      = baud rate (default 115200)
#
# Close the Arduino IDE Serial Monitor first -- the COM port can only be open
# in one program at a time.

import serial
import sys

if len(sys.argv) < 3:
    print("usage: python capture.py <port> <outfile> [stop_word] [baud]")
    print("example: python capture.py COM4 mylog.txt")
    sys.exit(1)

port = sys.argv[1]
outfile = sys.argv[2]
stop_word = sys.argv[3] if len(sys.argv) > 3 else "DONE"
baud = int(sys.argv[4]) if len(sys.argv) > 4 else 115200

use_stop = stop_word.upper() != "NONE"

ser = serial.Serial(port, baud, timeout=30)
print(f"listening on {port} @ {baud} -> {outfile}")
if use_stop:
    print(f"(will stop when a line contains '{stop_word}', or press Ctrl+C)")
else:
    print("(no stop marker -- press Ctrl+C to stop)")

try:
    with open(outfile, "w") as f:
        while True:
            line = ser.readline().decode(errors="replace").rstrip("\r\n")
            if not line:
                continue
            print(line)            # show it live
            f.write(line + "\n")
            f.flush()
            if use_stop and stop_word in line:
                break
except KeyboardInterrupt:
    print("\nstopped by user")
finally:
    ser.close()

print(f"saved -> {outfile}")
