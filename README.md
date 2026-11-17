# STM32G051F8P6 — OLED, klawiatura, ADC i DAC

## Piny (TSSOP20, zgodnie z rysunkiem)

| Funkcja | Pin |
|---|---|
| pin1: zwiększ wybrany DAC | PB7 |
| pin2: zmniejsz wybrany DAC | PC14 |
| pin3: wybierz DAC1/DAC2 | PC15 |
| pin4: wyzeruj wybrany DAC | PF2-NRST |
| OLED I2C2 SCL / SDA | PA11 / PA12, AF6 |
| ADC1 IN1, IN2, IN3, IN6, IN7 | PA1, PA2, PA3, PA6, PA7 |
| DAC1 OUT1 / OUT2 | PA4 / PA5 |
| SWDIO / SWCLK | PA13 / PA14 |
| zarezerwowane TIM2_CH1 / TIM3_CH2 | PA0 AF2 / PB3 AF1 |

Przyciski zwierają wejścia do GND (wewnętrzne pull-up). **PF2 wymaga
ustawienia opcji NRST_MODE=GPIO (wartość 2) w STM32CubeProgrammer.** Firmware
nie zapisuje option bytes. Przy funkcji resetu na tym pinie przycisk resetuje
procesor zamiast działać jako pin4. PC14/PC15 są GPIO, bez oscylatora LSE.
PA13/PA14 zachowują SWD; nie są przełączane na klawisze jak w projekcie G031.

OLED SSD1306 128×64 korzysta z przeniesionych sterownika i fontów G031.
Wykrywany jest pierwszy ekran pod adresem 7-bitowym 0x3C lub 0x3D
(0x78/0x7A w HAL). Potrzebne są zewnętrzne pull-up na SDA/SCL do 3,3 V.
Magistrala pracuje około 100 kHz przy PCLK=16 MHz. Brak OLED przy starcie
nie zatrzymuje ADC, DAC i klawiatury. Ekran odświeża się co 200 ms i pokazuje
surowe pomiary ADC oraz nastawy DAC; znak `>` oznacza wybrane wyjście.

## ADC i DAC

ADC1: 12 bitów, pięć kanałów w kolejności IN1, IN2, IN3, IN6, IN7,
ciągła konwersja, zegar 4 MHz, próbkowanie 160,5 cyklu.
Kalibracja przy starcie jest pominięta na czas diagnostyki zatrzymania
programu w `HAL_ADCEx_Calibration_Start`. Pomiary mogą mieć większy błąd
offsetu; pominięcie kalibracji nie potwierdza usunięcia przyczyny zatrzymania.
Około 4624 pełnych skanów/s. DMA1_Channel1 pracuje kołowo na 100 próbkach.
Przerwania połowy/końca bufora publikują ostatni ukończony skan; odczyt
pięciu wartości jest atomowy względem tych przerwań. Obsługa klawiatury
nie blokuje konwersji. Błąd ADC/DMA powoduje pokazywanie `----` zamiast
uznawania starych próbek za aktualne.

Dwa wyjścia DAC: 12 bitów, bufor wyjściowy włączony, bez zewnętrznego
wyzwalania. Startują od zera. Jedno naciśnięcie pin1/pin2 zmienia wybrany
DAC o 64, z ograniczeniem 0–4095; pin3 przełącza kanał, pin4 zeruje wybrany
kanał. Klawisze mają 30 ms eliminacji drgań, bez autorepetycji. Dodatkowe okno 100 ms pozwala zebrać kombinację kilku klawiszy przed
wykonaniem akcji; krótki nacisk jest obsługiwany po zwolnieniu. Kolejna
akcja wymaga zwolnienia wszystkich klawiszy. Sterowanie DAC działa na
ekranie ADC/DAC; na ekranie FM te same klawisze sterują radiem.
Nominalnie napięcie odpowiada `kod / 4095 * VREF+`; uwzględnij rzeczywiste
VREF+, obciążenie i ograniczenia bufora DAC. ADC pracuje w zakresie 0–VREF+.
Nie podłączaj napięć przekraczających dopuszczalne parametry mikrokontrolera.

PA0 i PB3 mają ustawione funkcje alternatywne z rysunku, ale timery nie są
uruchamiane — nie określono parametrów przebiegów ani trybu ich pracy.

## Wybór ekranów kombinacjami klawiszy

| Kombinacja | Ekran |
|---|---|
| pin1 + pin2 | ADC/DAC (również domyślny po starcie) |
| pin1 + pin3 | Stacje FM, nazwa i częstotliwość, bity dzielnika |
| pin2 + pin3 | Przykład 1 z G031: Radio krakow, symbole i prostokąty |
| pin1 + pin4 | Przykład 2: 1234+5!, tekst zwykły i odwrócony |
| pin2 + pin4 | Przykład 3: Rafa |
| pin3 + pin4 | Przykład 4: zestaw znaków na odwróconym tle |
| pin1 + pin2 + pin3 | Ekran tytułowy Radio FM / Synteza częstotliwości |

Przytrzymaj wybraną kombinację przez około 150–200 ms i zwolnij wszystkie
klawisze przed następną akcją. Cztery klatki z `EXAMPLE_DrawTxt` w G031 są
teraz osobnymi ekranami na jednym OLED. Nie ma pauzy 2300 ms ani przełączania
na drugi fizyczny wyświetlacz. Pomiary ADC trwają w tle na każdym ekranie,
a ustawienia DAC pozostają zachowane. Powrót do pomiarów: pin1+pin2.

## Radio FM i PCF8575

Na ekranie FM: pin1 = następna stacja, pin2 = poprzednia stacja,
pin3 = strojenie +0,01 MHz, pin4 = strojenie −0,01 MHz. Lista 24 stacji,
częstotliwości, dzielnik kroku `0x12BF` i wzór syntezy pochodzą z G031.
Strojenie jest zapamiętane osobno dla każdej stacji w RAM, w zakresie
±1 MHz. Po wyłączeniu zasilania wraca do wartości domyślnych.

Dwa **PCF8575 (16 bitów)** współdzielą I2C2 z OLED: 7-bitowy adres `0x20`
przyjmuje dzielnik kroku, `0x21` dzielnik częstotliwości. Wysyłane są dwa
bajty, najpierw młodszy, bez adresu rejestru, jak w G031. Dodano też odczyt
16-bitowego portu przez `PCF8575_Read`. To nie jest protokół PCF8574.

Wejście na ekran FM oraz każda akcja strojenia/wyboru stacji zapisują oba
ekspandery. Pozostałe ekrany nie wysyłają nastaw syntezy. Błąd komunikacji
jest pokazany jako `PCF ERR: 20`, `21` lub oba adresy; nie zatrzymuje ADC
ani interfejsu. Ponowne wejście na FM lub naciśnięcie klawisza FM ponawia
zapis. Dwa zapisy I2C nie są atomowe: przy błędzie jeden ekspander może mieć
już nową nastawę, a drugi poprzednią. Widoczna częstotliwość jest wartością
zadaną, nie pomiarem ani potwierdzeniem działania fizycznego odbiornika.

## Budowanie i testy

Wymagane: GNU Arm Embedded GCC z newlib-nano, GNU make, Python 3 i hostowy
kompilator C do testu klawiszy. W przygotowanym środowisku:

```sh
cd /workspace/STM32G051
export PATH=/workspace/stm32-cloud/toolchain/usr/bin:$PATH
make -j4
python3 tests/test_keys.py
python3 tests/test_radio.py
python3 tests/test_screens.py
```

Wyniki: `build/STM32G051.elf`, `.hex`, `.bin`, `.map`. Makefile obsługuje
zależności nagłówków i nie zmienia zapisanych w repozytorium plików Debug.
Sprawdzono GCC 14.2.1. Test hostowy wykonuje rzeczywisty kod GPIO z atrapą HAL:
mapowanie czterech pinów, drgania, przytrzymanie, zwolnienie, kombinacje
oraz przepełnienie licznika czasu. Test radia sprawdza listę stacji,
ograniczenia strojenia, wzór dzielnika, adresy/kolejność bajtów PCF8575
i obsługę błędów transmisji. Test ekranów renderuje cztery przykłady
i ekran FM przez rzeczywiste fonty/sterownik SSD1306 do atrapy wyświetlacza;
sprawdza powtarzalność kolorów, adresy OLED i widoczność błędów PCF.
Testy nie zastępują weryfikacji na płytce.

Konfigurację pinów i peryferiów odzwierciedla `STM32G051.ioc`; nie wykonano
regeneracji w CubeMX w środowisku chmurowym. Inicjalizacja ADC/DAC/DMA jest
w `Core/Src/analog.c`, wraz z ich przerwaniami. Przy regeneracji z CubeMX
należy uzgodnić wygenerowane inicjalizacje/przerwania z tym modułem, aby
nie powstały podwójne definicje ani ponowne inicjalizacje. W CubeIDE należy
odświeżyć projekt i przebudować pliki make (zapisane Debug pochodzą z Windows).

Do weryfikacji na sprzęcie: option bytes PF2, odczyt wszystkich klawiszy,
OLED i pull-up I2C, napięcia ADC na pięciu kanałach, oba napięcia DAC oraz SWD.
