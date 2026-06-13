# Sterownik Rolet v2.0 — Dokumentacja Techniczna

## 1. Informacje ogólne

| Parametr | Wartość |
|----------|---------|
| Procesor | AVR ATmega328P (Arduino Nano/Uno) |
| Taktowanie | 16 MHz |
| Język | C++ (Arduino Framework) |
| Przeznaczenie | Sterowanie jedną roletą zewnętrzną (góra/dół) |
| Sterowanie | Przyciski fizyczne + protokół Modbus RTU (RS-485) |
| Wersja firmware | 2.0 |

Urządzenie steruje jedną roletą za pomocą dwóch przekaźników (góra/dół). Roleta posiada wyłączniki krańcowe — detekcja końca przejazdu odbywa się poprzez pomiar prądu czujnikami ACS712 (prąd = 0 → krańcówka otwarta). Pozycja pośrednia jest wyznaczana programowo na podstawie czasu pracy motoru. Czasy przejazdu mogą być kalibrowane automatycznie i zapisywane w pamięci EEPROM.

---

## 2. Rozpiska pinów (Pinout)

### 2.1 Wyjścia — Przekaźniki

| Pin | Funkcja | Poziom aktywny | Opis |
|-----|---------|----------------|------|
| A0 | UP_RELAY | HIGH (1) | Przekaźnik kierunku „góra" (otwieranie rolety) |
| A1 | DOWN_RELAY | HIGH (1) | Przekaźnik kierunku „dół" (zamykanie rolety) |

**Logika przekaźników:**
- `digitalWrite(pin, HIGH)` → przekaźnik WŁĄCZONY (motor pracuje)
- `digitalWrite(pin, LOW)` → przekaźnik WYŁĄCZONY (motor zatrzymany)
- Oba piny konfigurowane jako `OUTPUT`
- Stan początkowy po starcie: oba `LOW` (wyłączone)

> **UWAGA BEZPIECZEŃSTWA:** Nigdy nie wolno załączyć obu przekaźników jednocześnie. Przy przełączaniu kierunku firmware najpierw wyłącza aktywny przekaźnik, odczekuje 10 ms, a dopiero potem załącza drugi.

### 2.2 Wejścia — Przyciski

| Pin | Funkcja | Tryb pinu | Poziom aktywny |
|-----|---------|-----------|----------------|
| A3 | Przycisk UP | INPUT_PULLUP | LOW (aktywny w stanie niskim) |
| A2 | Przycisk DOWN | INPUT_PULLUP | LOW (aktywny w stanie niskim) |

**Logika przycisków:**
- Przycisk wciśnięty → pin = `LOW` (pressed)
- Przycisk zwolniony → pin = `HIGH` (released)
- Wewnętrzne rezystory pull-up aktywowane programowo
- Debouncing: 25 ms (biblioteka Bounce2)

### 2.3 Wejścia — Czujniki prądu ACS712

| Pin | Funkcja | Typ czujnika | Czułość |
|-----|---------|--------------|---------|
| A4 | Czujnik prądu kanału UP | ACS712-05B (5A) | 185 mV/A |
| A5 | Czujnik prądu kanału DOWN | ACS712-05B (5A) | 185 mV/A |

**Detekcja krańcówek:**
- Czujniki mierzą prąd AC w obwodzie motoru metodą peak-to-peak
- Okno pomiarowe: 200 ms (~4 cykle 50Hz)
- Próg detekcji: `ACS_THRESHOLD = 15` jednostek ADC (do dobrania doświadczalnie)
- Po starcie motoru 1000 ms opóźnienia (ignorowanie stanów przejściowych)
- Peak-to-peak < próg → prąd nie płynie → krańcówka osiągnięta

### 2.4 Komunikacja szeregowa

| Pin | Funkcja |
|-----|---------|
| D0 (RX) | UART RX — odbiór danych Modbus RTU |
| D1 (TX) | UART TX — wysyłanie danych Modbus RTU |

- Magistrala RS-485 wymaga konwertera TTL ↔ RS-485 (np. MAX485)
- Pin sterujący kierunkiem RS-485 (DE/RE): **-1** (auto-direction)

---

## 3. Komunikacja Modbus RTU

### 3.1 Parametry komunikacji

| Parametr | Wartość |
|----------|---------|
| Protokół | Modbus RTU (slave) |
| Prędkość | 9600 baud |
| Parzystość | Brak (8N1) |
| Adres slave | Konfigurowalny, domyślnie **50** (zakres 50–80) |
| Identyfikator | "BLIND" (Report Server ID, funkcja 0x11) |

### 3.2 Mapa rejestrów Holding Registers (HR)

| HR | Kierunek | Nazwa | Wartości | Opis |
|----|----------|-------|----------|------|
| 0 | **Zapis** | Komenda | 0, 2, 4, 6 | Rozkaz sterujący (auto-czyszczony po przetworzeniu) |
| 1 | **Odczyt** | Stan | 0–5 | Aktualny stan urządzenia |
| 2 | **Zapis** | Pozycja docelowa | 0–100 | Zmiana wartości wyzwala pozycjonowanie |
| 3 | **Odczyt** | Pozycja aktualna | 0–100 | Estymacja pozycji w czasie rzeczywistym |
| 4 | **Zapis** | Adres Modbus | 50–80 | Nowy adres → EEPROM + auto-restart |
| 5 | **Odczyt** | UP_TIME | jednostki 100ms | Skalibrowany czas otwarcia |
| 6 | **Odczyt** | DOWN_TIME | jednostki 100ms | Skalibrowany czas zamknięcia |

### 3.3 Rejestr 0 — Komenda (HR 0, zapis)

| Wartość | Komenda | Opis |
|---------|---------|------|
| 0 | STOP | Zatrzymaj motor / przerwij kalibrację |
| 2 | OPEN | Otwórz roletę (pełny przejazd w górę) |
| 4 | CLOSE | Zamknij roletę (pełny przejazd w dół) |
| 6 | CALIBRATE | Rozpocznij procedurę kalibracji |

**Zachowanie:**
- Rejestr jest **automatycznie zerowany** po przetworzeniu komendy
- Dzięki temu tę samą komendę można wysłać ponownie bez konieczności wpisywania pośredniej wartości
- Komendy OPEN/CLOSE/CALIBRATE są ignorowane podczas trwającej kalibracji (poza STOP)

### 3.4 Rejestr 1 — Stan (HR 1, odczyt)

| Wartość | Stan | Opis |
|---------|------|------|
| 0 | STOP | Motor zatrzymany |
| 1 | OPENING | Motor pracuje w kierunku góra |
| 2 | OPEN | Roleta w pełni otwarta (pozycja = 0) |
| 3 | CLOSING | Motor pracuje w kierunku dół |
| 4 | CLOSED | Roleta w pełni zamknięta (pozycja = 100) |
| 5 | CALIBRATING | Trwa procedura kalibracji |

### 3.5 Rejestr 2 — Pozycja docelowa (HR 2, zapis)

- Zakres: 0–100 (procent zamknięcia)
- 0 = w pełni otwarta, 100 = w pełni zamknięta
- **Zmiana wartości** wyzwala automatyczne pozycjonowanie
- Czas jazdy obliczany proporcjonalnie do różnicy pozycji
- Kierunek dobierany automatycznie (target < current → UP, target > current → DOWN)
- Pozycjonowanie kończy się gdy: upłynie obliczony czas LUB ACS wykryje krańcówkę
- Wartości > 100 są ignorowane
- Aby wysłać tę samą pozycję ponownie, master musi najpierw wpisać inną wartość

### 3.6 Rejestr 3 — Pozycja aktualna (HR 3, odczyt)

- Zakres: 0–100 (procent zamknięcia)
- Aktualizowana w czasie rzeczywistym podczas ruchu (estymacja na podstawie czasu)
- Po wykryciu krańcówki korygowana do 0 lub 100

### 3.7 Rejestr 4 — Adres Modbus (HR 4, zapis)

- Zakres: 50–80
- Po zapisie poprawnego adresu (różnego od aktualnego):
  1. Motor zostaje zatrzymany
  2. Kalibracja zostaje przerwana
  3. Nowy adres jest zapisywany do EEPROM
  4. Urządzenie **automatycznie się restartuje**
- Po restarcie urządzenie komunikuje się na nowym adresie
- Wartości poza zakresem 50–80 lub równe aktualnemu adresowi są ignorowane

### 3.8 Rejestry 5–6 — Czasy kalibracji (HR 5–6, odczyt)

- HR 5: UP_TIME — czas pełnego otwarcia w jednostkach 100 ms (np. 300 = 30s)
- HR 6: DOWN_TIME — czas pełnego zamknięcia w jednostkach 100 ms
- Wartości domyślne: 300 (30 sekund)
- Aktualizowane po pomyślnej kalibracji

---

## 4. Pamięć EEPROM

### 4.1 Layout

| Offset | Rozmiar | Typ | Zawartość | Domyślnie |
|--------|---------|-----|-----------|-----------|
| 0 | 1 B | uint8_t | Magic byte | 0xA5 = dane poprawne |
| 1 | 1 B | uint8_t | Adres Modbus | 50 |
| 2–5 | 4 B | uint32_t | UP_TIME (ms) | 30000 |
| 6–9 | 4 B | uint32_t | DOWN_TIME (ms) | 30000 |

### 4.2 Walidacja

- Magic ≠ 0xA5 → wszystkie wartości domyślne
- Adres poza 50–80 → adres = 50
- Czasy poza 1000–120000 ms → czas = 30000 ms

### 4.3 Zapis

- **Kalibracja** — po pomyślnym zakończeniu (magic + UP_TIME + DOWN_TIME)
- **Zmiana adresu** — natychmiast (magic + adres), po czym auto-restart
- Używana funkcja `EEPROM.update()` — zapisuje tylko zmienione bajty (ochrona przed zużyciem)

### 4.4 Odczyt adresu

Adres jest ładowany z EEPROM **podczas inicjalizacji zmiennych globalnych** (przed `main()`) za pomocą niskopoziomowej funkcji `eeprom_read_byte()`. Dzięki temu obiekt `ModbusSerial` jest konstruowany z poprawnym adresem bez potrzeby dynamicznej alokacji.

---

## 5. Maszyna stanów

### 5.1 Stany urządzenia

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
     [pozycja=0]      [pozycja=100]

     CMD_CALIBRATE → CALIBRATING (5)
```

### 5.2 Tryby ruchu (wewnętrzne)

| Tryb | Opis | Wyzwalacz | Zakończenie |
|------|------|-----------|-------------|
| NONE | Motor zatrzymany | — | — |
| MANUAL | Przycisk trzymany | Wciśnięcie jednego przycisku | Puszczenie przycisku LUB safety timeout LUB ACS |
| AUTO | Ruch czasowy | Modbus OPEN/CLOSE, pozycjonowanie, oba przyciski | Timer LUB ACS |
| CALIBRATE | Kalibracja | Modbus CALIBRATE | Zakończenie procedury LUB safety timeout |

---

## 6. Procedura kalibracji

Wyzwalana przez zapis wartości `6` do HR 0.

```
Faza 1: GOING_UP_INIT
  Motor UP → czekaj na krańcówkę (ACS = 0)

Faza 2: PAUSE_1 (500 ms)
  Motor zatrzymany, odczekanie przed zmianą kierunku

Faza 3: GOING_DOWN
  Motor DOWN, stoper startuje
  Czekaj na krańcówkę → zapamiętaj DOWN_TIME

Faza 4: PAUSE_2 (500 ms)
  Motor zatrzymany, odczekanie

Faza 5: GOING_UP_FINAL
  Motor UP, stoper startuje
  Czekaj na krańcówkę → zapamiętaj UP_TIME

Zakończenie:
  Zapisz czasy do EEPROM
  Pozycja = 0 (otwarta), stan = OPEN
```

**Zabezpieczenia:**
- Pauzy realizowane nieblokująco (timer w pętli głównej)
- Safety timeout: 90 sekund na każdą fazę ruchu
- Przerwanie: przycisk fizyczny LUB komenda STOP z Modbus

---

## 7. Sterowanie przyciskami

### 7.1 Mapowanie

| Przycisk | Pin | Kierunek |
|----------|-----|----------|
| UP (A3) | A3 | Motor jedzie **w górę** (otwieranie) |
| DOWN (A2) | A2 | Motor jedzie **w dół** (zamykanie) |

### 7.2 Tryby interakcji

| Warunek | Akcja |
|---------|-------|
| Jeden przycisk, motor stoi | Hold-to-run: motor pracuje dopóki przycisk trzymany |
| Jeden przycisk, auto-move aktywny | STOP (przerwanie autoMove) |
| Oba przyciski | Pełny przejazd w ostatnim kierunku (autoMove) |
| Oba zwolnione | STOP (jeśli tryb ręczny) |
| Dowolny przycisk podczas kalibracji | Przerwanie kalibracji |

### 7.3 Safety timeout

- Ręczne sterowanie (hold-to-run) ma timeout = upTime/downTime
- Po przekroczeniu → automatyczny stop

### 7.4 Priorytet

- Przyciski mają **wyższy priorytet** niż komendy Modbus
- W pętli głównej przyciski są przetwarzane po Modbus — mogą nadpisać jego akcje
- Podczas kalibracji dowolny przycisk natychmiast ją przerywa

### 7.5 Flaga `waitFlag`

- Zapobiega wielokrotnemu wyzwalaniu akcji
- Ustawiana po: wciśnięciu obu przycisków, przerwaniu autoMove, przerwaniu kalibracji
- Resetowana gdy oba przyciski są zwolnione

---

## 8. Obliczanie pozycji

### 8.1 Zasada

Pozycja wyznaczana na podstawie czasu pracy motoru:

```
pozycja [%] = procent zamknięcia (0 = otwarta, 100 = zamknięta)

Podczas ruchu:
    percentMoved = min(elapsed × 100 / totalTime, 100)
    
    Otwieranie: pos = currentPosition - percentMoved
    Zamykanie:  pos = currentPosition + percentMoved
    
    clamp(pos, 0, 100)
```

### 8.2 Estymacja w czasie rzeczywistym

Rejestr HR 3 jest aktualizowany w każdej iteracji pętli głównej z estymacją pozycji (nie czeka na zatrzymanie).

### 8.3 Korekta pozycji

Po wykryciu krańcówki przez ACS712 pozycja jest korygowana definitywnie:
- Krańcówka górna → pozycja = 0
- Krańcówka dolna → pozycja = 100

---

## 9. Parametry czasowe

| Parametr | Wartość | Opis |
|----------|---------|------|
| UP_TIME | 30000 ms (domyślnie, kalibrowalny) | Czas pełnego otwarcia |
| DOWN_TIME | 30000 ms (domyślnie, kalibrowalny) | Czas pełnego zamknięcia |
| Debounce przycisków | 25 ms | Interwał filtrowania drgań |
| Pauza przełączania kierunku | 10 ms | Dead-time między relay off→on |
| Watchdog Timer | 2 s (WDTO_2S) | Timeout resetu WDT |
| Okno pomiaru prądu ACS | 200 ms | Okres peak-to-peak |
| Startup grace ACS | 1000 ms | Ignorowanie ACS po starcie motoru |
| Pauza kalibracji | 500 ms | Między zmianami kierunku |
| Safety timeout kalibracji | 90 s | Max czas na fazę ruchu |

---

## 10. Zabezpieczenia

| Zabezpieczenie | Opis |
|----------------|------|
| Watchdog Timer (2s) | Reset procesora przy zawieszeniu programu |
| WDT init3 disable | Zapobiega boot-loop po restarcie z WDT |
| Dead-time przekaźników | 10 ms między wyłączeniem a włączeniem |
| Safety timeout (manual) | Ręczne sterowanie ograniczone do UP/DOWN_TIME |
| Safety timeout (kalibracja) | 90s na fazę, po czym abort |
| Detekcja krańcówek ACS712 | Zatrzymanie motoru gdy prąd = 0 |
| Walidacja EEPROM | Magic byte + sprawdzenie zakresów |
| Walidacja adresu | Zakres 50–80, domyślnie 50 |
| Walidacja czasów | Zakres 1–120s, domyślnie 30s |
| Priorytet przycisków | Fizyczne przyciski nadpisują Modbus |
| EEPROM.update() | Zapis tylko zmienionych bajtów |

---

## 11. Cykl pracy pętli głównej

```
1. Reset Watchdog Timer
2. Obsługa komunikacji Modbus (slave.task())
3. Aktualizacja ACS712 (tylko gdy motor aktywny)
4. Aktualizacja maszyny stanów kalibracji
5. Przetwarzanie komend Modbus
6. Obsługa przycisków fizycznych (wyższy priorytet)
7. Sprawdzenie timeout autoMove
8. Sprawdzenie timeout ręcznego sterowania
9. Sprawdzenie detekcji krańcówek ACS712
10. Aktualizacja rejestrów Modbus wyjściowych
```

---

## 12. Zależności zewnętrzne

| Biblioteka | Źródło | Zastosowanie |
|------------|--------|-------------|
| ModbusSerial | [epsilonrt/modbus-serial](https://github.com/epsilonrt/modbus-serial) | Modbus RTU slave |
| Bounce2 | [thomasfredericks/Bounce2](https://github.com/thomasfredericks/Bounce2) | Debouncing przycisków |
| EEPROM | Arduino Core | Odczyt/zapis pamięci nieulotnej |
| avr/wdt.h | AVR LibC | Watchdog Timer |

---

## 13. Schemat blokowy hardware

```
                    ┌─────────────────────────────────┐
                    │       ATmega328P (Arduino)       │
                    │                                  │
    RS-485 Bus ◄───►│ D0 (RX)          A0 (UP_RELAY) │──► Moduł przekaźnikowy ──► Motor ↑
    (Modbus RTU)    │ D1 (TX)          A1 (DOWN_RELAY)│──► Moduł przekaźnikowy ──► Motor ↓
                    │                                  │
                    │                  A2 (DOWN_BTN)   │◄── Przycisk DOWN
                    │                  A3 (UP_BTN)     │◄── Przycisk UP
                    │                                  │     (pull-up wewnętrzny)
                    │                  A4 (I_SENSE_UP) │◄── ACS712 (kanał UP)
                    │                  A5 (I_SENSE_DN) │◄── ACS712 (kanał DOWN)
                    └─────────────────────────────────┘
```

---

## 14. Komunikacja Modbus — przykładowe sekwencje

### Otwieranie rolety
```
Master: Zapisz HR[0] = 2       → Slave: motor UP
Master: Odczytaj HR[1]         ← Slave: 1 (OPENING)
Master: Odczytaj HR[3]         ← Slave: 65 (estymacja pozycji)
... ACS wykrywa krańcówkę ...
Master: Odczytaj HR[1]         ← Slave: 2 (OPEN)
Master: Odczytaj HR[3]         ← Slave: 0
```

### Pozycjonowanie do 50%
```
Master: Zapisz HR[2] = 50      → Slave: oblicza kierunek i czas, start
Master: Odczytaj HR[1]         ← Slave: 1 lub 3 (zależy od kierunku)
Master: Odczytaj HR[3]         ← Slave: estymacja pozycji
... timer lub ACS ...
Master: Odczytaj HR[1]         ← Slave: 0 (STOP)
Master: Odczytaj HR[3]         ← Slave: 50
```

### Kalibracja
```
Master: Zapisz HR[0] = 6       → Slave: rozpoczyna kalibrację
Master: Odczytaj HR[1]         ← Slave: 5 (CALIBRATING)
... ~2 minuty (3 przejazdy) ...
Master: Odczytaj HR[1]         ← Slave: 2 (OPEN)
Master: Odczytaj HR[5]         ← Slave: nowy UP_TIME (np. 280 = 28s)
Master: Odczytaj HR[6]         ← Slave: nowy DOWN_TIME (np. 250 = 25s)
```

### Zmiana adresu
```
Master (adres 50): Zapisz HR[4] = 55   → Slave: EEPROM + restart
... urządzenie restartuje się ...
Master (adres 55): Odczytaj HR[1]      ← Slave: 0 (STOP)
```
