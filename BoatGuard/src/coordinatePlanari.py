# Possibile script python sempre in ascolto
# attende che la seriale invii il trigger "TRIGGER_START"
# per iniziare a salvare i dati e plottarli in tempo realegit 

import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

SERIAL_PORT = '/dev/ttyUSB0'  # o 'COM3' su Windows
BAUD_RATE = 115200

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

points_x = []
points_y = []

# Attesa del trigger per salvare i dati dalla seriale
recording = False  

def update(frame):
    global recording

    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line:
            if line == "TRIGGER_START":
                print("Ricevuto TRIGGER, inizio a salvare i dati...")
                recording = True
                return  

            if recording:
                tokens = line.split()
                if len(tokens) == 2:
                    try:
                        x = float(tokens[0])
                        y = float(tokens[1])
                        points_x.append(x)
                        points_y.append(y)
                    except ValueError:
                        pass

    plt.cla() 
    plt.scatter(points_x, points_y, c='b', marker='o')
    plt.title("Traiettoria barca")
    plt.xlabel("Asse X")
    plt.ylabel("Asse Y")
    plt.xlim(-50, 50)
    plt.ylim(-50, 50)

if __name__ == "__main__":
    fig = plt.figure()
    ani = FuncAnimation(fig, update, interval=100)
    plt.show()
