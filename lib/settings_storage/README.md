# Settings Storage moduļa dokumentācija

## Apraksts
Šis modulis nodrošina ESP32 iestatījumu saglabāšanu/ielādi izmantojot Preferences bibliotēku.

## Funkcijas
- `initSettingsStorage()` - Inicializē preferences un ielādē iepriekš saglabātos iestatījumus
- `saveAllSettings()` - Saglabā visus iestatījumus (temperatūras, dempfera un kontroles)
- `loadAllSettings()` - Ielādē visus iestatījumus no pastāvīgās atmiņas

## Lietošana
Iestatījumu saglabāšanas poga ir pieejama iestatījumu ekrānā. Lai saglabātu visus iestatījumus, nospiediet pogu "Saglabāt iestatījumus".
Saglabātie iestatījumi automātiski tiks ielādēti pie sistēmas pārstartēšanas.

## Datu struktūra
Modulis saglabā šādus iestatījumus:
- Temperatūras iestatījumi (mērķa temp., min. temp., maks. temp., brīdinājuma temp.)
- Dempfera iestatījumi (min. dempfers, maks. dempfers, servo leņķis utt.)
- Kontroles iestatījumi (PID parametri, beigu trigeri utt.)
