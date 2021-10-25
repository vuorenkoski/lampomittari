# Lämpötilanäyttö ja mittauspiste

## Hardware
- board: Lolin(Wemos) D1 mini pro, ESP8266
- näyttö: Nokia5110
- sensori: lämpötilavastus
- wifi (integroitu mikrokontrolleriin)

## Arduinon kirjastot
- ESP8266 board: https://arduino-esp8266.readthedocs.io/en/latest/installing.html
- Adafruit GFX library (versio 1.10.12)
- Adarfuit PCD8544 Nokia 5110 LCD library (versio 1.2.1)

## Toiminta
Mittaa lampotilan lampovastuksella ja lähettää sen serverille. Näytölle tulostetaan mitattu lampo, ulkolampotila ja järven lämpötila. Ulkolämpötila haetaan Ilmatieteen laitoksen vaoimesta datasta. Järven lämpö omalta serveriltä.

Sisälämpötila lähetetään lisäksi omalle serverille.

![image](kuva.jpg)
