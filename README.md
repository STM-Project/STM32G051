# STM32G051F8P6 — OLED, klawiatura, ADC i DAC

## Piny (TSSOP20, zgodnie z rysunkiem)

| Funkcja | Pin |
|---|---|
| pin1: zwiększ wybrany DAC | PB7 |
| pin2: zmniejsz wybrany DAC | PC14 |
| pin3: wybierz następny z sześciu DAC | PC15 |
| pin4: wyzeruj wybrany DAC | PF2-NRST |
| OLED i MCP4728A1 I2C2 SCL / SDA | PA11 / PA12, AF6 |
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
nie zatrzymuje ADC, DAC i klawiatury. OLED pokazuje sześć pomiarów ADC,
dwie częstotliwości w jednym wierszu (PA0 po lewej, PB3 po prawej)
i sześć nastaw DAC. Każde pole częstotliwości mieści pięć cyfr i dopisek
`Hz`. Wynik powyżej zakresu jest wyświetlany jako `>99999Hz`. Nastawy DAC zajmują dwie linie
po trzy liczby bez etykiet. Kolejność od lewej do prawej: u góry PA4, PA5,
MCP4728 A, u dołu MCP4728 B, C, D. Po starcie wszystkie nastawy są wyświetlane zwykłym tekstem. Pierwsze
naciśnięcie pin1, pin2 lub pin3 zaznacza PA4 w negatywie. Kolejne naciśnięcia
pin3 wybierają następne wyjście. Po MCP4728 D kolejne naciśnięcie usuwa
zaznaczenie, a następne wybiera ponownie PA4. Nastawy mieszczą się
w zakresie 0–4095.

Błąd komunikacji na I2C2 z OLED lub MCP4728 zastępuje bieżący ekran
komunikatem `ERROR I2C` i wierszem `OLED`, `MCP4728A1` lub
`MCP4728A1 + OLED`. Firmware ponawia inicjalizację OLED co 250 ms.
Jeżeli transmisja do OLED jest niemożliwa, komunikat pojawi się po odzyskaniu
łączności. Po przesłaniu komunikatu firmware wyświetla go przez co najmniej
2 s i wraca do bieżącego ekranu, gdy zapisy DAC ponownie działają.

## ADC i DAC

ADC1: 12 bitów, pięć kanałów w kolejności IN1, IN2, IN3, IN6, IN7,
ciągła konwersja, zegar 4 MHz, próbkowanie 160,5 cyklu.
Przy starcie firmware kalibruje offset ADC przed włączeniem DMA i offset
buforów obu kanałów DAC przed włączeniem wyjść. Jeśli kalibracja ADC
nie powiedzie się, a ADC nadal jest w stanie bezczynności, firmware
uruchamia standardową konwersję. Jeśli ADC pozostaje zajęty lub DMA
nie wystartuje, urządzenie działa dalej, a OLED pokazuje `----` dla ADC.
W razie błędu kalibracji OLED pokazuje przez 2 s komunikat
`BRAK KALIBRACJI` i wskazuje ADC1 (wspólną kalibrację dla wszystkich
wejść ADC) lub odpowiedni kanał DAC. Kalibracja zmniejsza błąd zera.
Dokładność wzmocnienia, napięcie VREF+ i wpływ obciążenia wyjścia
wymagają osobnej weryfikacji.

DMA1_Channel1 pracuje w trybie Circular:
Peripheral to Memory, half-word (16 bitów), Memory Increment włączone,
Peripheral Increment wyłączone, priorytet High. ADC ma włączone Continuous
Conversion i DMA Continuous Requests. Bufor zawiera 100 próbek (20 skanów),
ma wyrównanie 32-bitowe i jest oznaczony volatile dla odczytów z DMA.
W `.ioc` przypisanie DMA zapisane jest w sekcji `Dma.*` (wielkość liter ma
znaczenie); włączone są przerwania DMA1_Channel1 i ADC1_COMP.
Przerwania połowy/końca bufora publikują ostatni ukończony skan; odczyt
pięciu wartości jest atomowy względem tych przerwań. Obsługa klawiatury
nie blokuje konwersji. Błąd ADC/DMA powoduje pokazywanie `----` zamiast
uznawania starych próbek za aktualne.

Dwa wyjścia DAC: 12 bitów, bufor wyjściowy włączony, bez zewnętrznego
wyzwalania. Startują od zera. Jedno naciśnięcie pin1/pin2 zmienia wybrany
DAC o 1, z ograniczeniem 0–4095. Jeśli klawisz pozostaje wciśnięty przez
sekundę po pierwszej zmianie, kolejne kroki o 1 są wykonywane co około 5 ms,
zależnie od czasu obsługi OLED. Po przytrzymaniu przycisku przez ponad 5 s
krok DAC wzrasta do 10, z ograniczeniem do 0–4095. Zwolnienie przycisku
kończy powtarzanie.
Pin3 przechodzi przez sześć kanałów i stan bez zaznaczenia.
Pin4 zeruje wybrany kanał. Klawisze mają 8 ms
eliminacji drgań i 40 ms na zebranie kombinacji. Krótki nacisk jest
obsługiwany po zwolnieniu. Przed kolejną akcją zwolnij wszystkie klawisze.
Powtarzanie działa tylko dla pin1/pin2 na ekranie ADC/DAC. Na ekranie FM
te same klawisze sterują radiem.
Nominalnie napięcie odpowiada `kod / 4095 * VREF+`; uwzględnij rzeczywiste
VREF+, obciążenie i ograniczenia bufora DAC. ADC pracuje w zakresie 0–VREF+.
Nie podłączaj napięć przekraczających dopuszczalne parametry mikrokontrolera.

PA0 i PB3 mają ustawione funkcje alternatywne z rysunku, ale timery nie są
uruchamiane — nie określono parametrów przebiegów ani trybu ich pracy.

### MCP4728A1-E/UN

Podłącz SCL do PA11, SDA do PA12 i połącz masy. Zasil układ napięciem
3,3 V, użyj podciągania magistrali do 3,3 V i podłącz LDAC do GND.
MCP4728A1 ma fabryczny adres 7-bitowy `0x61` (`0xC2` w HAL)
i współdzieli I2C2 z OLED i PCF8575.

`MCP4728_WriteChannel(channel, value)` przyjmuje numer kanału A–D (0–3)
i 12-bitowy kod. Wysyła trzy bajty w jednej transakcji I²C: polecenie
Multi-Write `0x40 | (channel << 1)` z UDAC=0, starsze cztery bity kodu
i młodszy bajt kodu. Zapis dotyczy jednego kanału i od razu aktualizuje
wyjście. Referencją jest VDD, wzmocnienie wynosi 1, a wyjście jest aktywne.
Nominalne napięcie wynosi `VDD * kod / 4096`.

Po starcie firmware wpisuje zera do rejestrów czterech wyjść.
Do pierwszego zapisu obowiązują ustawienia startowe układu zapisane w EEPROM.
Zmiany nastaw trafiają do rejestrów ulotnych. Po błędzie I²C aplikacja
ponawia zapis aktualnej nastawy po 250 ms i nadal obsługuje pozostałe peryferia.

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
| pin1 + pin2 + pin4 | Animacja twarzy (10 s) |
| pin1 + pin3 + pin4 | Ręczne sterowanie wyjściami PCF8575 |

Naciśnij klawisze kombinacji razem i zwolnij wszystkie przed następną akcją.
Cztery klatki z `EXAMPLE_DrawTxt` w G031 są teraz osobnymi ekranami na jednym OLED. Nie ma pauzy 2300 ms ani przełączania
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

Na ekranie ręcznego sterowania pary bajtów są pokazane w HEX, starszy
bajt po lewej. Górny rząd kółek odpowiada adresowi `0x20`, dolny `0x21`.
W każdym rzędzie bity biegną od 15 do 0. Pełne kółko oznacza 1, puste 0.
Kółka służą do podglądu, a podkreślenie wskazuje wybrany bajt HEX.
Pin1 zwiększa bajt o 1, pin2 zmniejsza go o 1, pin3 wybiera następny bajt,
a pin4 zeruje wybrany bajt. Wartość zatrzymuje się na `00` lub `FF`.
Przytrzymanie pin1 lub pin2 przez sekundę uruchamia szybkie powtarzanie
tak jak przy ustawianiu DAC.

Edytor przechowuje własne nastawy w RAM. Po restarcie zaczynają się od
`FF FF` dla obu układów. Wejście do edytora wpisuje nastawy ręczne, a wejście
na ekran FM wpisuje zapamiętane dzielniki wybranej stacji. Oba ekrany korzystają
z tych samych fizycznych wyjść, lecz przechowują osobne nastawy. Błąd transmisji
jest oznaczany jako `ERR` z adresem układu.

## Budowanie i testy

Wymagane: GNU Arm Embedded GCC z newlib-nano, GNU make, Python 3 i hostowy
kompilator C do testu klawiszy. W przygotowanym środowisku:

```sh
cd /workspace/STM32G051
export PATH=/workspace/stm32-cloud/toolchain/usr/bin:$PATH
export C_INCLUDE_PATH=/workspace/stm32-cloud/toolchain/usr/include/newlib/nano${C_INCLUDE_PATH:+:$C_INCLUDE_PATH}
make -j4
python3 tests/test_keys.py
python3 tests/test_radio.py
python3 tests/test_screens.py
python3 tests/test_mcp4728.py
python3 tests/test_adc_dma.py
python3 tests/test_regeneration.py
```

Wyniki: `build/STM32G051.elf`, `.hex`, `.bin`, `.map`. Makefile obsługuje
zależności nagłówków i nie zmienia zapisanych w repozytorium plików Debug.
Sprawdzono GCC 14.2.1. Test hostowy wykonuje rzeczywisty kod GPIO z atrapą HAL:
mapowanie czterech pinów, drgania, przytrzymanie, zwolnienie, kombinacje,
szybkie powtarzanie DAC i przepełnienie licznika czasu. Test radia sprawdza
listę stacji, ograniczenia strojenia, wzór dzielnika, adresy/kolejność bajtów PCF8575
i obsługę błędów transmisji. Test ekranów renderuje cztery przykłady
i ekran FM przez rzeczywiste fonty/sterownik SSD1306 do atrapy wyświetlacza;
sprawdza powtarzalność kolorów, adresy OLED i widoczność błędów PCF.
Test ADC/DMA wykonuje rzeczywisty moduł analogowy z atrapą HAL: sprawdza
konfigurację kołową, kolejność pięciu rang, start transferu, kolejne obroty
obu połówek bufora, spójność odczytu, obsługę przerwań i błędów.
Testy nie zastępują weryfikacji na płytce.

## Regeneracja CubeMX

Plik `STM32G051.ioc` oparto na wersji przesłanej po regeneracji w CubeMX
6.16.1. Zachowaj `Keep User Code when re-generating` (`KeepUserCode=true`)
oraz generowanie osobnych plików `.c/.h` peryferiów (`CoupleFile=true`).

`MX_DMA_Init`, `MX_ADC1_Init`, `MX_DAC1_Init` i `MX_I2C2_Init` są w modułach
`dma.c`, `adc.c`, `dac.c`, `i2c.c`. Wspólne uchwyty `hadc1`, `hdma_adc1`,
`hdac1` mają jedną definicję w tych modułach. Main wywołuje wygenerowane
inicjalizacje przed `USER CODE BEGIN 2`, w kolejności GPIO, DMA, ADC, DAC, I2C.
W USER CODE 2 pozostaje tylko uruchomienie aplikacji, DMA i wyjść DAC przez
`Analog_Init`, inicjalizacja OLED i pierwsze rysowanie. Kalibracja jest wykonywana w
`Analog_Init` przed uruchomieniem ADC i wyjść DAC.

Funkcje wektorów `DMA1_Channel1_IRQHandler` i `ADC1_COMP_IRQHandler` są
wyłącznie w `stm32g0xx_it.c` i korzystają z tych samych uchwytów co aplikacja.
`analog.c` zawiera bufor i callbacki ADC, bez duplikatów wektorów ani MSP.
Obsługa klawiatury jest w `USER CODE BEGIN 2` w gpio.c, jej deklaracje
w `USER CODE BEGIN Prototypes` w gpio.h. Deklaracje transportu OLED w i2c.h
również są w sekcji USER CODE. Pozostałe moduły aplikacji są osobnymi plikami.

Po generacji wykonaj Clean i przebuduj projekt w CubeIDE. Test
`test_regeneration.py` sprawdza parowanie sekcji, pojedyncze definicje IRQ
i uruchamia test klawiszy po modelowej regeneracji gpio.c/gpio.h zachowującej
wyłącznie USER CODE. Nie wykonuje samego CubeMX; rzeczywistą regenerację
należy potwierdzić lokalnie w IDE. Przesłana konfiguracja nie aktywuje TIM2
ani TIM3; timery nie są uruchamiane przez aplikację. Rezerwacja PA0/PB3
jako funkcji alternatywnych z rysunku pozostaje w USER CODE MspInit 1.

Do weryfikacji na sprzęcie: option bytes PF2, odczyt wszystkich klawiszy,
OLED i pull-up I2C, napięcia ADC na pięciu kanałach, oba napięcia DAC oraz SWD.
