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
ciągła konwersja, kalibracja przy starcie, zegar 4 MHz, próbkowanie 160,5 cyklu.
Około 4624 pełnych skanów/s. DMA1_Channel1 pracuje kołowo na 100 próbkach.
Przerwania połowy/końca bufora publikują ostatni ukończony skan; odczyt
pięciu wartości jest atomowy względem tych przerwań. Obsługa klawiatury
nie blokuje konwersji. Błąd ADC/DMA powoduje pokazywanie `----` zamiast
uznawania starych próbek za aktualne.

Dwa wyjścia DAC: 12 bitów, bufor wyjściowy włączony, bez zewnętrznego
wyzwalania. Startują od zera. Jedno naciśnięcie pin1/pin2 zmienia wybrany
DAC o 64, z ograniczeniem 0–4095; pin3 przełącza kanał, pin4 zeruje wybrany
kanał. Klawisze mają 30 ms eliminacji drgań, bez autorepetycji. Kombinacje
klawiszy są ignorowane; kolejna akcja wymaga zwolnienia wszystkich klawiszy.
Nominalnie napięcie odpowiada `kod / 4095 * VREF+`; uwzględnij rzeczywiste
VREF+, obciążenie i ograniczenia bufora DAC. ADC pracuje w zakresie 0–VREF+.
Nie podłączaj napięć przekraczających dopuszczalne parametry mikrokontrolera.

PA0 i PB3 mają ustawione funkcje alternatywne z rysunku, ale timery nie są
uruchamiane — nie określono parametrów przebiegów ani trybu ich pracy.

## Budowanie i testy

Wymagane: GNU Arm Embedded GCC z newlib-nano, GNU make, Python 3 i hostowy
kompilator C do testu klawiszy. W przygotowanym środowisku:

```sh
cd /workspace/STM32G051
export PATH=/workspace/stm32-cloud/toolchain/usr/bin:$PATH
make -j4
python3 tests/test_keys.py
```

Wyniki: `build/STM32G051.elf`, `.hex`, `.bin`, `.map`. Makefile obsługuje
zależności nagłówków i nie zmienia zapisanych w repozytorium plików Debug.
Sprawdzono GCC 14.2.1. Test hostowy wykonuje rzeczywisty kod GPIO z atrapą HAL:
mapowanie czterech pinów, drgania, przytrzymanie, zwolnienie, kombinacje
oraz przepełnienie licznika czasu. Nie zastępuje testu elektrycznego płytki.

Konfigurację pinów i peryferiów odzwierciedla `STM32G051.ioc`; nie wykonano
regeneracji w CubeMX w środowisku chmurowym. Inicjalizacja ADC/DAC/DMA jest
w `Core/Src/analog.c`, wraz z ich przerwaniami. Przy regeneracji z CubeMX
należy uzgodnić wygenerowane inicjalizacje/przerwania z tym modułem, aby
nie powstały podwójne definicje ani ponowne inicjalizacje. W CubeIDE należy
odświeżyć projekt i przebudować pliki make (zapisane Debug pochodzą z Windows).

Do weryfikacji na sprzęcie: option bytes PF2, odczyt wszystkich klawiszy,
OLED i pull-up I2C, napięcia ADC na pięciu kanałach, oba napięcia DAC oraz SWD.
