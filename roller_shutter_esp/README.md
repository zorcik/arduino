# Roller Shutter Controller v2.0 — ESP32-C3

Firmware do sterowania roletą zewnętrzną oparty na mikrokontrolerze **ESP32-C3** (RISC-V, single-core). Sterowanie odbywa się za pomocą przycisków fizycznych lub protokołu **Modbus RTU** (RS-485).

---

## Spis treści

1. [Informacje ogólne](#1-informacje-ogólne)
2. [Hardware](#2-hardware)
3. [Rozpiska pinów](#3-rozpiska-pinów)
4. [Flagi kompilacji](#4-flagi-kompilacji)
5. [Komunikacja Modbus RTU](#5-komunikacja-modbus-rtu)
6. [Komendy Modbus](#6-komendy-modbus)
7. [Sterowanie przyciskami](#7-sterowanie-przyciskami)
8. [Obliczanie pozycji](#8-obliczanie-pozycji)
9. [Detekcja krańcówek (ACS712)](#9-detekcja-krańcówek-acs712)
10. [Kalibracja](#10-kalibracja)
11. [Pamięć EEPROM](#11-pamięć-eeprom)
12. [Debug — port szeregowy](#12-debug--port-szeregowy)
13. [Maszyna stanów](#13-maszyna-stanów)
14. [Cykl pętli głównej](#14-cykl-pętli-głównej)
15. [Parametry czasowe](#15-parametry-czasowe)
16. [Zabezpieczenia](#16-zabezpieczenia)
17. [Zależności](#17-zależności)
18. [Przykłady sekwencji Modbus](#18-przykłady-sekwencji-modbus)

---

## 1. Informacje ogólne

| Parametr | Wartość |
|----------|---------|
| Procesor | ESP32-C3 (RISC-V, single-core, 160 MHz) |
| Framework | Arduino |
| Przeznaczenie | Sterowanie jedną roletą (góra/dół) |
| Sterowanie | Przyciski fizyczne + Modbus RTU (RS-485) |
| Czujnik prądu | ACS712 (opcjonalny) |
| WiFi/BLE | Wyłączone programowo |
| Wersja firmware | 2.0 |

---

## 2. Hardware

### Schemat blokowy

```
                    ┌──────────────────────────────────────┐
                    │           ESP32-C3                    │
                    │                                      │
    RS-485 Bus ◄───►│ GPIO10 (RX)     GPIO5/0 (UP_RELAY)  │──► Moduł przekaźnikowy ──► Motor ↑
    (Modbus RTU)    │ GPIO20 (TX)     GPIO1  (DOWN_RELAY)  │──► Moduł przekaźnikowy ──► Motor ↓
                    │                                      │
                    │                 GPIO2   (DOWN_BTN)    │◄── Przycisk DOWN
                    │                 GPIO3   (UP_BTN)      │◄── Przycisk UP
                    │                                      │
                    │                 GPIO4   (ACS_UP)      │◄── ACS712 (kanał UP)
                    │                 GPIO0/5 (ACS_DOWN)    │◄── ACS712 (kanał DOWN)
                    │                                      │
    USB / UART0 ◄──►│ Debug serial (115200 baud)           │
                    └──────────────────────────────────────┘
```

### Wersje płytki (HARDWARE_VERSION)

Stała `HARDWARE_VERSION` definiuje przypisanie pinów GPIO. Zmiana była konieczna, ponieważ GPIO 5 na niektórych wersjach ESP32-C3 nie obsługuje ADC (kanał ADC2 niedostępny, gdy WiFi jest używane, lub brak obsługi na niektórych wariantach).

| Wersja | UP_RELAY | DOWN_RELAY | DOWN_BTN | UP_BTN | ACS_UP | ACS_DOWN |
|--------|----------|------------|----------|--------|--------|----------|
| < 42 | GPIO 0 | GPIO 1 | GPIO 2 | GPIO 3 | GPIO 4 | GPIO 5 |
| ≥ 42 | **GPIO 5** | GPIO 1 | GPIO 2 | GPIO 3 | GPIO 4 | **GPIO 0** |

W wersji ≥ 42 piny GPIO 0 i GPIO 5 zostały zamienione — ACS_DOWN przeniesiony na GPIO 0 (ADC1_CH0), a UP_RELAY na GPIO 5 (który nie wymaga ADC).

---

## 3. Rozpiska pinów

### Wyjścia — Przekaźniki

| Pin (HW ≥ 42) | Funkcja | Poziom aktywny | Opis |
|----------------|---------|----------------|------|
| GPIO 5 | UP_RELAY | HIGH (1) | Przekaźnik kierunku „góra" (otwieranie) |
| GPIO 1 | DOWN_RELAY | HIGH (1) | Przekaźnik kierunku „dół" (zamykanie) |

- `HIGH` = przekaźnik włączony (motor pracuje)
- `LOW` = przekaźnik wyłączony (motor zatrzymany)
- Stan początkowy: oba `LOW`
- **Nigdy** nie włączać obu jednocześnie — dead-time 10 ms przy przełączaniu

### Wejścia — Przyciski

| Pin | Funkcja | Tryb | Aktywny |
|-----|---------|------|---------|
| GPIO 3 | Przycisk UP | INPUT_PULLUP | LOW (wciśnięty) |
| GPIO 2 | Przycisk DOWN | INPUT_PULLUP | LOW (wciśnięty) |

- Debouncing: 25 ms (biblioteka Bounce2)
- Pull-up wewnętrzny aktywny

> ⚠️ **GPIO 2** jest pinem strapping na ESP32-C3. Jeśli przycisk DOWN jest wciśnięty podczas uruchamiania, może to wpłynąć na tryb bootowania.

### Wejścia analogowe — ACS712

| Pin (HW ≥ 42) | Funkcja | Kanał ADC |
|----------------|---------|-----------|
| GPIO 4 | Czujnik prądu UP | ADC1_CH4 |
| GPIO 0 | Czujnik prądu DOWN | ADC1_CH0 |

- ADC: 12-bit (zakres 0–4095)
- Próg detekcji prądu: `ACS_THRESHOLD = 60` (do dobrania doświadczalnie)

### Porty szeregowe

| Port | Piny | Prędkość | Funkcja |
|------|------|----------|---------|
| UART0 (Serial) | USB | 115200 baud | Debug (opcjonalnie) |
| UART1 (modbusSerial) | GPIO 10 (RX), GPIO 20 (TX) | 9600 baud, 8N1 | Modbus RTU |

---

## 4. Flagi kompilacji

Stałe konfiguracyjne definiowane jako `#define` na początku pliku:

| Stała | Wartości | Domyślnie | Opis |
|-------|----------|-----------|------|
| `DEBUG_ENABLED` | `true` / `false` | `true` | Włącza komunikaty debug na Serial (USB) |
| `HARDWARE_VERSION` | liczba | `42` | Wersja płytki; ≥ 42 zamienia GPIO 0 ↔ GPIO 5 |
| `ACS_INSTALLED` | `true` / `false` | `true` | Czy czujniki ACS712 są zainstalowane |

### Kiedy `ACS_INSTALLED = false`:

- Pomijane wszystkie odczyty analogowe z pinów ACS
- `isEndStopReached()` zawsze zwraca `false`
- Komenda CALIBRATE jest ignorowana (nic nie robi)
- Motor zatrzymuje się **wyłącznie** po upłynięciu czasu (UP_TIME / DOWN_TIME)
- Pozycja wyznaczana tylko na podstawie czasu

### Kiedy `DEBUG_ENABLED = false`:

- Makro `DBG_PRINTF(...)` kompilowane jako puste — zero narzutu
- Serial (UART0) nie jest używany
- Oszczędność flash i CPU

---

## 5. Komunikacja Modbus RTU

### Parametry

| Parametr | Wartość |
|----------|---------|
| Protokół | Modbus RTU (slave) |
| Port | UART1 (HardwareSerial(1)) |
| Prędkość | 9600 baud |
| Format | 8N1 (8 bitów danych, bez parzystości, 1 bit stopu) |
| Piny | RX = GPIO 10, TX = GPIO 20 |
| Adres slave | Konfigurowalny, domyślnie **50** (zakres 50–80) |
| Identyfikator | "BLIND" (Report Server ID) |

### Mapa rejestrów Holding Registers (HR)

| HR | Adres | Kierunek | Nazwa | Zakres | Opis |
|----|-------|----------|-------|--------|------|
| 0 | 0x0000 | **Zapis** | Komenda | 0, 2, 4, 6 | Rozkaz sterujący |
| 1 | 0x0001 | **Odczyt** | Stan | 0–5 | Aktualny stan urządzenia |
| 2 | 0x0002 | **Zapis** | Pozycja docelowa | 0–100 | Wyzwala pozycjonowanie |
| 3 | 0x0003 | **Odczyt** | Pozycja aktualna | 0–100 | Estymacja w czasie rzeczywistym |
| 4 | 0x0004 | **Zapis** | Adres Modbus | 50–80 | Nowy adres → EEPROM + restart |
| 5 | 0x0005 | **Odczyt** | UP_TIME | jedn. 100ms | Czas otwarcia (np. 300 = 30s) |
| 6 | 0x0006 | **Odczyt** | DOWN_TIME | jedn. 100ms | Czas zamknięcia |

---

## 6. Komendy Modbus

### HR 0 — Rejestr komend (zapis)

Komendę wysyła się zapisując wartość do rejestru HR 0 (adres 0x0000) funkcją Modbus **Write Single Register (0x06)** lub **Write Multiple Registers (0x10)**.

| Wartość | Komenda | Opis |
|---------|---------|------|
| **0** | STOP | Zatrzymaj motor. Przerywa kalibrację. |
| **2** | OPEN | Otwórz roletę (pełny przejazd w górę). |
| **4** | CLOSE | Zamknij roletę (pełny przejazd w dół). |
| **6** | CALIBRATE | Rozpocznij procedurę kalibracji (wymaga ACS712). |

**Zachowanie rejestru komend:**
- Rejestr jest **automatycznie zerowany** (→ 0 / STOP) po przetworzeniu komendy
- Dzięki temu tę samą komendę można wysłać ponownie bez konieczności wpisywania pośredniej wartości
- Komendy OPEN/CLOSE/CALIBRATE są ignorowane podczas trwającej kalibracji (poza STOP)
- Jeśli motor już pracuje, nowa komenda OPEN/CLOSE najpierw go zatrzymuje, a potem uruchamia w nowym kierunku

#### Komenda STOP (wartość 0)

- Zatrzymuje motor niezależnie od trybu pracy (manualny, automatyczny)
- Przerywa kalibrację jeśli jest w toku
- Aktualizuje pozycję na podstawie czasu pracy motoru

#### Komenda OPEN (wartość 2)

- Uruchamia motor w kierunku **góra** (otwieranie)
- Tryb: automatyczny z timeoutem = UP_TIME
- Pozycja docelowa: 0% (w pełni otwarta)
- Zatrzymanie: po upłynięciu UP_TIME lub detekcji krańcówki (ACS)
- Ignorowana podczas kalibracji

#### Komenda CLOSE (wartość 4)

- Uruchamia motor w kierunku **dół** (zamykanie)
- Tryb: automatyczny z timeoutem = DOWN_TIME
- Pozycja docelowa: 100% (w pełni zamknięta)
- Zatrzymanie: po upłynięciu DOWN_TIME lub detekcji krańcówki (ACS)
- Ignorowana podczas kalibracji

#### Komenda CALIBRATE (wartość 6)

- Rozpoczyna automatyczną kalibrację (patrz sekcja [10. Kalibracja](#10-kalibracja))
- **Wymaga `ACS_INSTALLED = true`** — w przeciwnym razie ignorowana
- Przerywa aktualny ruch przed rozpoczęciem
- Jeśli kalibracja jest już w toku, najpierw ją przerywa i rozpoczyna od nowa

### HR 1 — Stan urządzenia (odczyt)

| Wartość | Stan | Opis |
|---------|------|------|
| 0 | STOP | Motor zatrzymany |
| 1 | OPENING | Motor jedzie w górę |
| 2 | OPEN | Roleta w pełni otwarta (pozycja = 0%) |
| 3 | CLOSING | Motor jedzie w dół |
| 4 | CLOSED | Roleta w pełni zamknięta (pozycja = 100%) |
| 5 | CALIBRATING | Trwa procedura kalibracji |

### HR 2 — Pozycja docelowa (zapis)

- **Zakres**: 0–100 (procent zamknięcia)
  - `0` = w pełni otwarta
  - `100` = w pełni zamknięta
  - `50` = w połowie
- **Wyzwalacz**: zmiana wartości rejestru rozpoczyna ruch
- Kierunek dobierany automatycznie:
  - target < aktualna pozycja → motor UP
  - target > aktualna pozycja → motor DOWN
- Czas jazdy obliczany proporcjonalnie do różnicy pozycji
- Wartości > 100 są ignorowane
- Aby wysłać tę samą pozycję ponownie, master musi najpierw wpisać inną wartość

### HR 3 — Pozycja aktualna (odczyt)

- **Zakres**: 0–100 (procent zamknięcia)
- Aktualizowana w **każdej iteracji pętli głównej** (estymacja w czasie rzeczywistym)
- Wartość bazowa aktualizowana przy zatrzymaniu motoru
- Po wykryciu krańcówki (ACS) korygowana definitywnie do 0 lub 100

### HR 4 — Adres Modbus (zapis)

- **Zakres**: 50–80
- Po zapisie poprawnego adresu (różnego od aktualnego):
  1. Motor zostaje zatrzymany
  2. Kalibracja zostaje przerwana
  3. Nowy adres jest zapisywany do EEPROM (flash)
  4. Urządzenie **automatycznie się restartuje** (`ESP.restart()`)
- Po restarcie urządzenie komunikuje się na nowym adresie
- Wartości poza zakresem 50–80 lub równe aktualnemu adresowi są ignorowane
- **UWAGA**: Po zmianie adresu master musi komunikować się na nowym adresie

### HR 5 — UP_TIME (odczyt)

- Skalibrowany czas pełnego otwarcia w **jednostkach 100 ms**
- Przykład: wartość `300` = 30 000 ms = 30 sekund
- Domyślnie: 300 (30s)
- Aktualizowany po pomyślnej kalibracji

### HR 6 — DOWN_TIME (odczyt)

- Skalibrowany czas pełnego zamknięcia w **jednostkach 100 ms**
- Przykład: wartość `250` = 25 000 ms = 25 sekund
- Domyślnie: 300 (30s)
- Aktualizowany po pomyślnej kalibracji

---

## 7. Sterowanie przyciskami

### Mapowanie

| Przycisk | Pin | Kierunek |
|----------|-----|----------|
| UP | GPIO 3 | Motor jedzie **w górę** (otwieranie) |
| DOWN | GPIO 2 | Motor jedzie **w dół** (zamykanie) |

### Tryby interakcji

| Warunek | Akcja |
|---------|-------|
| Jeden przycisk wciśnięty, motor stoi | **Hold-to-run**: motor pracuje dopóki przycisk jest trzymany |
| Jeden przycisk wciśnięty, auto-move aktywny | **STOP** — przerwanie auto-move (toggle) |
| Oba przyciski wciśnięte | **Pełny przejazd** w ostatnim kierunku ruchu (auto-move) |
| Oba przyciski zwolnione | **STOP** (jeśli tryb ręczny hold-to-run) |
| Dowolny przycisk podczas kalibracji | **Przerwanie kalibracji** |

### Hold-to-run (tryb manualny)

- Motor pracuje tak długo, jak przycisk jest trzymany
- Po zwolnieniu przycisku motor natychmiast się zatrzymuje
- Safety timeout: ograniczony do UP_TIME / DOWN_TIME (zależnie od kierunku)
- Po przekroczeniu safety timeout → automatyczny stop

### Oba przyciski jednocześnie

- Wyzwala pełny przejazd (auto-move) w **ostatnim kierunku** ruchu
- Jeśli motor stał i nie było wcześniejszego ruchu → kierunek UP (domyślny)
- Flaga `waitFlag` blokuje powtórzenie do momentu zwolnienia obu przycisków

### Priorytet

- Przyciski mają **wyższy priorytet** niż komendy Modbus
- W pętli głównej przyciski są przetwarzane PO Modbus — mogą nadpisać jego akcje
- Podczas kalibracji dowolny przycisk natychmiast ją przerywa

---

## 8. Obliczanie pozycji

### Definicja

- **Pozycja 0%** = roleta w pełni otwarta (krańcówka górna)
- **Pozycja 100%** = roleta w pełni zamknięta (krańcówka dolna)

### Algorytm

Pozycja jest estymowana na podstawie czasu pracy motoru:

```
percentMoved = min(elapsed × 100 / totalTime, 100)

Otwieranie (UP):   pos = currentPosition - percentMoved
Zamykanie (DOWN):  pos = currentPosition + percentMoved

clamp(pos, 0, 100)
```

- `elapsed` — czas od uruchomienia motoru (`millis() - moveStartTime`)
- `totalTime` — skalibrowany czas pełnego przejazdu (UP_TIME lub DOWN_TIME)

### Estymacja w czasie rzeczywistym

Funkcja `getEstimatedPosition()` jest wywoływana w każdej iteracji pętli głównej i zwraca bieżącą estymację pozycji. Rejestr HR 3 jest aktualizowany z tą wartością.

### Korekta pozycji

Gdy ACS712 wykryje krańcówkę (prąd = 0), pozycja jest korygowana definitywnie:
- Krańcówka górna → pozycja = **0%**
- Krańcówka dolna → pozycja = **100%**

To koryguje ewentualne błędy kumulowane z estymacji czasowej.

### Pozycjonowanie (HR 2)

Przy pozycjonowaniu do zadanej pozycji:
1. Wyznaczana jest różnica: `diff = |target - current|`
2. Czas ruchu: `moveTime = diff × totalTime / 100`
3. Po upłynięciu czasu pozycja jest ustawiana na dokładną wartość docelową

---

## 9. Detekcja krańcówek (ACS712)

### Zasada działania

Roleta posiada mechaniczne wyłączniki krańcowe. Gdy roleta osiągnie pozycję skrajną, wyłącznik odcina obwód motoru i prąd spada do zera. Czujnik ACS712 mierzy prąd metodą **peak-to-peak** (amplituda AC).

### Algorytm

1. W każdej iteracji pętli odczytywana jest wartość ADC z odpowiedniego kanału (UP lub DOWN, zależnie od kierunku ruchu)
2. Śledzone są wartości `acsMax` i `acsMin` w oknie pomiarowym
3. Co 200 ms (≈4 cykle 50 Hz) obliczany jest peak-to-peak: `pp = acsMax - acsMin`
4. Jeśli `pp < ACS_THRESHOLD` → prąd nie płynie → krańcówka osiągnięta

### Parametry

| Parametr | Wartość | Opis |
|----------|---------|------|
| `ACS_THRESHOLD` | 60 | Min. wartość peak-to-peak ADC (12-bit, 0–4095) |
| `ACS_WINDOW_MS` | 200 ms | Okno pomiarowe (~4 cykle 50 Hz) |
| `ACS_STARTUP_DELAY_MS` | 1000 ms | Okres karencji po starcie motoru |

### Startup grace period

Po uruchomieniu motoru przez 1000 ms pomiary ACS są ignorowane. Zapobiega to fałszywym detekcjom krańcówek spowodowanym stanami przejściowymi (inrush current, drgania mechaniczne).

### Strojenie progu ACS_THRESHOLD — procedura doświadczalna

Wartość `ACS_THRESHOLD = 60` jest **domyślna** i **musi** zostać dobrana doświadczalnie dla konkretnej instalacji. Próg zależy od:
- typu i mocy motoru rolety
- długości przewodów między czujnikiem a motorem
- wariantu czujnika ACS712 (5A / 20A / 30A)
- szumów elektromagnetycznych w instalacji

#### Wymagania

- `DEBUG_ENABLED = true`
- `ACS_INSTALLED = true`
- Komputer podłączony do portu USB z monitorem portu szeregowego (115200 baud)
- Roleta podłączona i gotowa do pracy

#### Krok 1 — Pomiar pp podczas normalnej pracy motoru

1. Otwórz monitor portu szeregowego (np. Arduino Serial Monitor, PuTTY, minicom)
2. Wyślij komendę **OPEN** przez Modbus (HR[0] = 2) lub wciśnij przycisk UP
3. Obserwuj logi `[ACS]` — interesuje Cię wartość `pp` (peak-to-peak):

```
[ACS] pp=287 thr=60 flowing=YES grace=YES
[ACS] pp=312 thr=60 flowing=YES grace=YES
[ACS] pp=295 thr=60 flowing=YES grace=YES
[ACS] pp=301 thr=60 flowing=YES grace=YES
[ACS] Startup grace period ended
[ACS] pp=290 thr=60 flowing=YES grace=no
[ACS] pp=305 thr=60 flowing=YES grace=no
[ACS] pp=278 thr=60 flowing=YES grace=no
```

4. Zanotuj kilkanaście wartości pp. Oblicz **minimalną** wartość pp podczas normalnej pracy:

```
Przykład: pp = 278, 287, 290, 295, 301, 305, 312
→ pp_min (praca) = 278
```

5. Powtórz dla kierunku **DOWN** (komenda CLOSE lub przycisk DOWN)
6. Użyj **mniejszej** z dwóch wartości pp_min (UP i DOWN)

#### Krok 2 — Pomiar pp przy krańcówce (brak prądu)

Gdy roleta dojdzie do krańcówki mechanicznej, wyłącznik odcina obwód i prąd spada do zera. Odczytaj wartości pp w tym momencie:

```
[ACS] pp=285 thr=60 flowing=YES grace=no
[ACS] pp=292 thr=60 flowing=YES grace=no
[ACS] pp=12 thr=60 flowing=NO grace=no      ← krańcówka!
[ACS] pp=5 thr=60 flowing=NO grace=no
[ACS] pp=8 thr=60 flowing=NO grace=no
[ACS] End-stop reached: dir=UP, pos=0
```

Zanotuj wartości pp po detekcji krańcówki:

```
Przykład: pp = 5, 8, 12
→ pp_max (krańcówka) = 12
```

> **Uwaga**: Jeśli roleta ma domyślny `ACS_THRESHOLD` ustawiony zbyt wysoko i nie wykrywa krańcówki (motor jedzie aż do safety timeout), tymczasowo ustaw `ACS_THRESHOLD = 1000` i powtórz pomiary — firmware nie będzie reagował na ACS, ale logi będą pokazywać rzeczywiste wartości pp.

#### Krok 3 — Wyznaczenie progu

Próg powinien znajdować się **pomiędzy** wartością pp przy krańcówce a wartością pp podczas normalnej pracy, z marginesem bezpieczeństwa:

```
ACS_THRESHOLD = pp_max(krańcówka) + (pp_min(praca) - pp_max(krańcówka)) × 0.3
```

Czyli 30% zakresu powyżej szumu krańcówkowego. Przykład:

```
pp_max (krańcówka) = 12
pp_min (praca)     = 278

Zakres = 278 - 12 = 266
Margines = 266 × 0.3 = 80

ACS_THRESHOLD = 12 + 80 = 92
```

W tym przykładzie optymalny próg to **~90**.

#### Krok 4 — Walidacja

1. Wgraj firmware z nową wartością `ACS_THRESHOLD`
2. Wykonaj pełny cykl otwarcia i zamknięcia
3. Sprawdź w logach:
   - Czy `flowing=YES` jest stabilne podczas normalnej pracy motoru
   - Czy `flowing=NO` pojawia się **dopiero** po osiągnięciu krańcówki
   - Czy nie ma fałszywych przejść `flowing=YES → NO → YES` podczas ruchu
4. Uruchom kalibrację (HR[0] = 6) i sprawdź czy procedura kończy się poprawnie

#### Diagnostyka problemów

| Objaw | Przyczyna | Rozwiązanie |
|-------|-----------|-------------|
| Motor zatrzymuje się w trakcie jazdy (fałszywa detekcja krańcówki) | `ACS_THRESHOLD` za wysoki | Zmniejsz próg |
| Motor nie wykrywa krańcówki (jedzie do safety timeout) | `ACS_THRESHOLD` za niski | Zwiększ próg |
| Wartości pp wahają się mocno (np. 50–400) | Szumy EMI, słabe ekranowanie | Dodaj kondensator filtrujący 100nF na wyjściu ACS712 |
| pp przy krańcówce > 0 (np. 20–40) | Szumy ADC, długie przewody | Normalny — uwzględnij w progu, wartość pp przy krańcówce to poziom szumów |
| pp przy krańcówce bliskie pp przy pracy | Czujnik ACS712 źle podłączony lub uszkodzony | Sprawdź okablowanie, napięcie referencyjne czujnika |
| Kalibracja kończy się natychmiast | Próg za wysoki — ACS natychmiast wykrywa „brak prądu" | Zmniejsz próg znacząco |
| Wartości pp = 0 zawsze | Pin ADC nie podłączony lub zły pin | Sprawdź HARDWARE_VERSION i fizyczne połączenie |

#### Typowe zakresy wartości

| Instalacja | pp (praca) | pp (krańcówka) | Sugerowany próg |
|------------|-----------|----------------|-----------------|
| Mały motor (np. 50W) | 80–200 | 3–15 | 30–50 |
| Średni motor (np. 150W) | 200–500 | 5–20 | 60–100 |
| Duży motor (np. 300W+) | 400–1000+ | 8–30 | 100–150 |

> **Uwaga**: Powyższe wartości są orientacyjne. Rzeczywiste wartości zależą od wariantu ACS712 (5A/20A/30A), impedancji obwodu i długości przewodów. **Zawsze** wykonaj pomiary na konkretnej instalacji.

#### Wpływ czynników zewnętrznych

- **Temperatura** — czujnik ACS712 ma dryft temperaturowy ~0.4%/°C. W skrajnych warunkach (−20°C / +50°C) szum może wzrosnąć. Jeśli roleta pracuje na zewnątrz, dodaj dodatkowy margines (~20%).
- **WiFi** — radio WiFi generuje szumy na ADC ESP32. W tym firmware WiFi jest wyłączone (`WiFi.mode(WIFI_OFF)`), co znacząco poprawia jakość odczytów.
- **Zasilacz** — impulsowy zasilacz może generować szumy na linii zasilania. Rozważ dodanie kondensatora elektrolitycznego 100µF + ceramicznego 100nF na zasilaniu czujnika ACS712.
- **Długość przewodów** — długie przewody między czujnikiem a ESP32 mogą zbierać zakłócenia. Skrócić do minimum lub użyć ekranowanego przewodu.

---

## 10. Kalibracja

### Wymagania

- `ACS_INSTALLED = true` (wymaga czujników prądu do detekcji krańcówek)
- Gdy `ACS_INSTALLED = false`, komenda CALIBRATE jest ignorowana

### Wyzwalanie

Zapis wartości `6` do rejestru HR 0 (komenda CALIBRATE).

### Procedura (5 faz)

```
Faza 1: GOING_UP_INIT
  Motor UP → czekaj na krańcówkę górną (ACS pp < threshold)
  
Faza 2: PAUSE_1 (500 ms)
  Motor zatrzymany, odczekanie przed zmianą kierunku

Faza 3: GOING_DOWN (pomiar DOWN_TIME)
  Motor DOWN, stoper startuje
  Czekaj na krańcówkę dolną → zapamiętaj DOWN_TIME

Faza 4: PAUSE_2 (500 ms)
  Motor zatrzymany, odczekanie

Faza 5: GOING_UP_FINAL (pomiar UP_TIME)
  Motor UP, stoper startuje
  Czekaj na krańcówkę górną → zapamiętaj UP_TIME

Zakończenie:
  Zapisz czasy do EEPROM (flash)
  Pozycja = 0% (otwarta), stan = OPEN
```

### Cechy implementacji

- **Nieblokująca** — pauzy realizowane jako timery w pętli głównej (bez `delay()`)
- **Safety timeout** — 90 sekund na każdą fazę ruchu; po przekroczeniu → abort
- **Przerwanie** — dowolny przycisk fizyczny LUB komenda STOP z Modbus
- **Wynik** — zmierzone czasy zapisywane do EEPROM, czytelne w HR 5 i HR 6

### Po kalibracji

- Czasy UP_TIME i DOWN_TIME są używane do:
  - Wyznaczania pozycji (estymacja czasowa)
  - Timeoutów auto-move i safety timeout manualnego sterowania
  - Obliczania czasu ruchu przy pozycjonowaniu (HR 2)
- Czasy zachowane w EEPROM przeżywają restart / odłączenie zasilania

---

## 11. Pamięć EEPROM

### Implementacja

Na ESP32 EEPROM jest emulowany we flash (NVS). Wymaga:
- `EEPROM.begin(size)` w `setup()` — alokacja bufora w RAM
- `EEPROM.commit()` po zapisach — zrzut bufora do flash

### Layout

| Offset | Rozmiar | Typ | Zawartość | Domyślnie |
|--------|---------|-----|-----------|-----------|
| 0x00 | 1 B | uint8_t | Magic byte | `0xA5` = dane poprawne |
| 0x01 | 1 B | uint8_t | Adres Modbus | 50 |
| 0x02–0x05 | 4 B | uint32_t | UP_TIME (ms) | 30000 |
| 0x06–0x09 | 4 B | uint32_t | DOWN_TIME (ms) | 30000 |

Łącznie: 10 bajtów (alokowane 16 B).

### Walidacja

| Warunek | Zachowanie |
|---------|------------|
| Magic ≠ `0xA5` | Wszystkie wartości domyślne |
| Adres poza 50–80 | Adres = 50 |
| Czas poza 1000–120000 ms | Czas = 30000 ms |

### Zapis

| Zdarzenie | Co jest zapisywane |
|-----------|-------------------|
| Kalibracja (sukces) | Magic + UP_TIME + DOWN_TIME |
| Zmiana adresu (HR 4) | Magic + adres, po czym auto-restart |

---

## 12. Debug — port szeregowy

### Konfiguracja

```cpp
#define DEBUG_ENABLED  true   // Włącz/wyłącz debug
```

Gdy `true`, komunikaty debug są wysyłane na **Serial** (UART0 / USB) z prędkością **115200 baud**.

### Prefiksy komunikatów

| Prefiks | Źródło | Opis |
|---------|--------|------|
| `[BOOT]` | `setup()` | Informacje o starcie i konfiguracji |
| `[EEPROM]` | Load/Save | Odczyt/zapis EEPROM |
| `[CMD]` | `processModbus()` | Odebrana komenda Modbus (STOP/OPEN/CLOSE/CALIBRATE) |
| `[POS]` | `processModbus()` | Pozycjonowanie — cel, kierunek, czas |
| `[ADDR]` | `processModbus()` | Zmiana adresu Modbus |
| `[ACS]` | `updateACS()` | Pomiar peak-to-peak, stan flowing, grace period |
| `[CAL]` | Calibration | Fazy kalibracji, zmierzone czasy |
| `[MOVE]` | `loop()` | Zakończenie ruchu (timeout, pozycja osiągnięta) |
| `[BTN]` | `processButtons()` | Akcje przycisków |
| `[REG]` | `loop()` | Zmiana stanu lub pozycji (drukowane tylko gdy się zmienią) |
| `[RESET]` | `softwareReset()` | Restart urządzenia |

### Przykładowy output

```
========================================
 ROLLER SHUTTER CONTROLLER v2.0
 MCU: ESP32-C3
========================================
[BOOT] Relays initialized (OFF)
[BOOT] Buttons initialized (A2/A3 with pull-up)
[EEPROM] Loaded: addr=50, UP=28500 ms, DOWN=26000 ms
[BOOT] Config: addr=50, UP=28500 ms, DOWN=26000 ms
[BOOT] Modbus initialized: addr=50, UART1 RX=10 TX=20
[BOOT] Setup complete. ACS installed: YES
========================================

[CMD] OPEN
[REG] state=1(OPENING) pos=0%
[ACS] pp=312 thr=60 flowing=YES grace=YES
[ACS] pp=287 thr=60 flowing=YES grace=YES
[ACS] pp=295 thr=60 flowing=YES grace=YES
[ACS] pp=301 thr=60 flowing=YES grace=YES
[ACS] Startup grace period ended
[ACS] pp=290 thr=60 flowing=YES grace=no
[REG] state=1(OPENING) pos=3%
...
[ACS] pp=8 thr=60 flowing=NO grace=no
[ACS] End-stop reached: dir=UP, pos=0
[REG] state=2(OPEN) pos=0%
```

---

## 13. Maszyna stanów

### Stany urządzenia

```
                STOP (0)
               ╱     ╲
     CMD_OPEN ╱       ╲ CMD_CLOSE
             ╱         ╲
      OPENING (1)    CLOSING (3)
        │                  │
        │ timeout/ACS      │ timeout/ACS
        ▼                  ▼
      OPEN (2)         CLOSED (4)
     [pozycja=0%]     [pozycja=100%]

     CMD_CALIBRATE → CALIBRATING (5)
```

### Tryby ruchu (wewnętrzne)

| Tryb | Opis | Wyzwalacz | Zakończenie |
|------|------|-----------|-------------|
| NONE | Motor zatrzymany | — | — |
| MANUAL | Hold-to-run | Przycisk fizyczny | Puszczenie przycisku / safety timeout / ACS |
| AUTO | Ruch czasowy | Modbus OPEN/CLOSE, pozycjonowanie, oba przyciski | Timer / ACS |
| CALIBRATE | Kalibracja | Modbus CALIBRATE | Zakończenie procedury / safety timeout |

---

## 14. Cykl pętli głównej

Pętla `loop()` wykonuje następujące kroki w każdej iteracji:

```
 1. Obsługa komunikacji Modbus (slave->task())
 2. Aktualizacja ACS712 — odczyt ADC, peak-to-peak (tylko gdy motor aktywny)
 3. Aktualizacja maszyny stanów kalibracji
 4. Przetwarzanie komend Modbus (HR 0, HR 2, HR 4)
 5. Obsługa przycisków fizycznych (wyższy priorytet)
 6. Sprawdzenie timeout auto-move (pełny przejazd / pozycjonowanie)
 7. Sprawdzenie timeout ręcznego sterowania (safety)
 8. Sprawdzenie detekcji krańcówek ACS712
 9. Aktualizacja rejestrów Modbus wyjściowych (HR 1, 3, 4, 5, 6)
10. Debug: drukowanie zmian stanu/pozycji
```

---

## 15. Parametry czasowe

| Parametr | Wartość | Opis |
|----------|---------|------|
| `DEFAULT_UP_TIME` | 30 000 ms | Domyślny czas otwarcia (kalibrowalny) |
| `DEFAULT_DOWN_TIME` | 30 000 ms | Domyślny czas zamknięcia (kalibrowalny) |
| `DEBOUNCE_MS` | 25 ms | Filtrowanie drgań przycisków |
| `RELAY_SWITCH_DELAY_MS` | 10 ms | Dead-time przy przełączaniu kierunku |
| `ACS_WINDOW_MS` | 200 ms | Okno pomiaru prądu peak-to-peak |
| `ACS_STARTUP_DELAY_MS` | 1 000 ms | Karencja ACS po starcie motoru |
| `CAL_PAUSE_MS` | 500 ms | Pauza między fazami kalibracji |
| `CAL_SAFETY_TIMEOUT_MS` | 90 000 ms | Max czas na fazę ruchu kalibracji |

---

## 16. Zabezpieczenia

| Zabezpieczenie | Opis |
|----------------|------|
| Dead-time przekaźników | 10 ms między wyłączeniem a włączeniem — zapobiega zwarciu |
| Safety timeout (manual) | Ręczne sterowanie ograniczone do UP/DOWN_TIME |
| Safety timeout (kalibracja) | 90 s na fazę, po czym abort |
| Detekcja krańcówek ACS712 | Zatrzymanie motoru gdy prąd = 0 |
| Startup grace ACS | 1 s ignorowania odczytów po starcie motoru |
| Walidacja EEPROM | Magic byte + sprawdzenie zakresów wartości |
| Walidacja adresu | Zakres 50–80, domyślnie 50 |
| Walidacja czasów | Zakres 1–120 s, domyślnie 30 s |
| Priorytet przycisków | Fizyczne przyciski nadpisują komendy Modbus |
| WiFi wyłączone | `WiFi.mode(WIFI_OFF)` — oszczędność energii, brak zakłóceń ADC |

---

## 17. Zależności

| Biblioteka | Źródło | Wersja | Zastosowanie |
|------------|--------|--------|-------------|
| ModbusSerial | [epsilonrt/modbus-serial](https://github.com/epsilonrt/modbus-serial) | — | Modbus RTU slave |
| Bounce2 | [thomasfredericks/Bounce2](https://github.com/thomasfredericks/Bounce2) | — | Debouncing przycisków |
| EEPROM | Arduino ESP32 Core | — | Pamięć nieulotna (flash-backed) |
| WiFi | Arduino ESP32 Core | — | Wyłączenie radia WiFi |

### Wymagania platformowe

| Parametr | Wartość |
|----------|---------|
| Platforma | ESP32-C3 |
| Arduino Core | ESP32 Arduino Core 2.x lub 3.x |
| Board | Dowolna płytka ESP32-C3 (np. ESP32-C3-DevKitM-1) |

---

## 18. Przykłady sekwencji Modbus

### Otwieranie rolety

```
Master → Slave (addr 50): Write HR[0] = 2          (CMD_OPEN)
                           Slave: motor UP, auto-move

Master → Slave:            Read HR[1]
Slave → Master:                        HR[1] = 1    (OPENING)

Master → Slave:            Read HR[3]
Slave → Master:                        HR[3] = 65   (estymacja pozycji)
... czas mija / ACS wykrywa krańcówkę ...

Master → Slave:            Read HR[1]
Slave → Master:                        HR[1] = 2    (OPEN)

Master → Slave:            Read HR[3]
Slave → Master:                        HR[3] = 0    (w pełni otwarta)
```

### Pozycjonowanie do 50%

```
Master → Slave:            Write HR[2] = 50         (target 50%)
                           Slave: oblicza kierunek i czas, start

Master → Slave:            Read HR[1]
Slave → Master:                        HR[1] = 1 lub 3 (zależy od kierunku)

Master → Slave:            Read HR[3]
Slave → Master:                        HR[3] = 35   (estymacja w trakcie ruchu)
... timer lub ACS ...

Master → Slave:            Read HR[1]
Slave → Master:                        HR[1] = 0    (STOP)

Master → Slave:            Read HR[3]
Slave → Master:                        HR[3] = 50   (pozycja osiągnięta)
```

### Kalibracja

```
Master → Slave:            Write HR[0] = 6          (CMD_CALIBRATE)
                           Slave: rozpoczyna kalibrację

Master → Slave:            Read HR[1]
Slave → Master:                        HR[1] = 5    (CALIBRATING)
... ~2 minuty (3 przejazdy: UP, DOWN, UP) ...

Master → Slave:            Read HR[1]
Slave → Master:                        HR[1] = 2    (OPEN — kalibracja kończy się na górze)

Master → Slave:            Read HR[5]
Slave → Master:                        HR[5] = 285  (UP_TIME = 28.5s)

Master → Slave:            Read HR[6]
Slave → Master:                        HR[6] = 260  (DOWN_TIME = 26.0s)
```

### Zmiana adresu Modbus

```
Master (addr 50) → Slave:  Write HR[4] = 55         (nowy adres)
                           Slave: EEPROM → restart
... urządzenie restartuje się ...

Master (addr 55) → Slave:  Read HR[1]
Slave → Master:                        HR[1] = 0    (STOP — świeży boot)
```

### Odczyt konfiguracji

```
Master → Slave:            Read HR[4]
Slave → Master:                        HR[4] = 50   (aktualny adres)

Master → Slave:            Read HR[5]
Slave → Master:                        HR[5] = 300  (UP_TIME = 30.0s)

Master → Slave:            Read HR[6]
Slave → Master:                        HR[6] = 300  (DOWN_TIME = 30.0s)
```
