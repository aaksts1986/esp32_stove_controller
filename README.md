# Malkas Krāsns Vadības Panelis

Šis projekts ir izstrādāts kā vadības panelis malkas krāsnij, izmantojot ESP32, LVGL grafisko bibliotēku un TFT displeju. Sistēma nodrošina temperatūras uzraudzību, automātisku gaisa vārsta (damper) vadību, pieskāriena pogas funkcionalitāti un vizuālu atgriezenisko saiti uz displeja.

## Funkcionalitāte

- **Temperatūras sensors** – reāllaikā nolasa un attēlo krāsns temperatūru.
- **Automātiska vārsta vadība** – PID algoritms regulē gaisa padevi atkarībā no temperatūras un lietotāja iestatītās mērķa temperatūras.
- **Pieskāriena ekrāns** – lietotājs var mainīt mērķa temperatūru un saņemt vizuālu atgriezenisko saiti par pieskārieniem.
- **Displeja vizualizācijas** – temperatūras stabi, vārsta stāvokļa indikācija, pieskāriena punktu attēlošana.
- **Buzzer un relejs** – iespēja pievienot skaņas un elektriskos signālus (piemēram, brīdinājumiem vai citiem notikumiem).

## Aparatūras prasības

- ESP32 mikrokontrolleris
- TFT displejs (ar pieskāriena funkciju, piemēram, ILI9341)
- Temperatūras sensors (piemēram, DS18B20)
- Servo motors vārsta vadībai
- Pieskāriena poga (vai kapacitatīvais sensors)
- Buzzer un relejs (pēc izvēles)

## Programmatūras bibliotēkas

- [LVGL](https://lvgl.io/) – grafiskā bibliotēka
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) – displeja draiveris
- [ESP32Servo](https://github.com/jkb-git/ESP32Servo) – servo vadība
- [FreeRTOS](https://www.freertos.org/) – uzdevumu pārvaldība

## Projekta struktūra

- main.cpp – galvenā programmas plūsma, inicializācija un cikls
- lvgl_display – displeja un pieskāriena apstrāde
- damper_control – PID vadība un vārsta algoritmi
- temperature – temperatūras sensora apstrāde
- touch_button – pieskāriena pogas apstrāde

## Uzsākšana

1. Klonējiet šo repozitoriju.
2. Pielāgojiet platformio.ini atbilstoši jūsu aparatūrai.
3. Augšupielādējiet programmu uz ESP32.
4. Pievienojiet nepieciešamās ierīces (displejs, sensors, servo, u.c.).

## Ekrānšāviņi

![Vadības panelis](screenshot.png) <!-- Pievienojiet savu attēlu, ja nepieciešams -->

## Licencēšana

Šis projekts ir atvērta koda un pieejams saskaņā ar MIT licenci.

---
Papildināts fragments README.md sadaļai "Aparatūras pieslēgumi" un "Papildu iespējas":

---

## Aparatūras pieslēgumi

| Funkcija         | ESP32 PIN | Vada krāsa  | Piezīmes                        |
|------------------|-----------|-------------|----------------------------------|
| GND              | GND       | Melns       | Zeme                             |
| +5V              | 5V        | Sarkans     | Barošana                        |
| Servo            | 5         | Dzeltens    | Servo signāla vads               |
| Temperatūras sensors (OneWire) | 6         | Balts       | DS18B20 datu vads                |
| Touch Button 1   | 4         |             | Pieskāriena poga 1               |
| Touch Button 2   | 1         |             | Pieskāriena poga 2               |
| Touch Button 3   | 2         |             | Pieskāriena poga 3               |
| Relejs 1         | 17        | Zaļš        | Krāsns vadība                    |
| Relejs 2         | 16        | Oranžs      | Papildu vadība                   |
| Buzzer           | 14        |             | Skaņas signāls                   |
| TFT s led        | 8         | ...         | Skārienjutīgs, pieslēgums atbilstoši displeja dokumentācijai |

> **Piezīme:** Displejs ir skārienjutīgs un ļauj lietotājam mainīt iestatījumus tieši uz ekrāna.

---

## Papildu iespējas

- **Telegram integrācija:** Sistēma tiks savienota ar Telegram aplikāciju, lai attālināti saņemtu paziņojumus un vadītu krāsns darbību.
- **Skārienjutīgs ekrāns:** Visi iestatījumi un informācija ir pieejama tieši uz TFT displeja ar pieskāriena vadību.

---

Ja nepieciešams, vari šo tabulu un aprakstu iekļaut README.md sadaļā "Aparatūras pieslēgumi" un "Papildu iespējas".
Izstrādāts ar mīlestību Latvijas malkas krāsnīm!
