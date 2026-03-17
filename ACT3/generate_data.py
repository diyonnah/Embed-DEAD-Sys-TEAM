import csv
import random

rows = 50000

with open("environment_dataset_50k.csv","w",newline="") as file:

    writer = csv.writer(file)

    writer.writerow(["Temperature","Humidity","Label"])

    for i in range(rows):

        temp = random.randint(18,40)
        hum = random.randint(30,95)

        if temp > 30:
            label = "Hot"

        elif hum > 75:
            label = "Humid"

        else:
            label = "Comfortable"

        writer.writerow([temp,hum,label])

print("Dataset generated successfully.")
