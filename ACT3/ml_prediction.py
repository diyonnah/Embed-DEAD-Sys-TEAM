from machine import Pin
import dht
import math
import time

sensor = dht.DHT11(Pin(13))

# Replace these values with the ones obtained from training
w_temp = -8.420
w_hum = -0.709
bias = 306.606

def sigmoid(x):
    return 1/(1+math.exp(-x))
def predict(temp,hum):

    z = (w_temp * temp) + (w_hum * hum) + bias
    probability = sigmoid(z)

    if probability > 0.5:
        return "Hot/Humid"
    else:
        return "Comfortable"

while True:

    sensor.measure()

    temp = sensor.temperature()
    hum = sensor.humidity()

    prediction = predict(temp,hum)

    print("Temperature:",temp)
    print("Humidity:",hum)
    print("Prediction:",prediction)

    time.sleep(3)
