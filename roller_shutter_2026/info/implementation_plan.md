# Nowy Sterownik Rolet — Plan Implementacji

## Cel

Napisanie kompletnego nowego programu sterującego roletą dla ATmega328P z poprawionymi błędami i 4 nowymi funkcjami:

1. **Detekcja krańcówek** — via ACS712 (prąd = 0 → koniec jazdy)
2. **Kalibracja** — pomiar UP_TIME/DOWN_TIME, zapis do EEPROM
3. **Adres Modbus** — ustawianie via rejestr, zapis do EEPROM (zakres 50–80)
4. **Pozycjonowanie** — jazda do zadanej pozycji 0–100%

## Poprawione błędy z analizy

| # | Błąd | Poprawka |
|---|------|----------|
| 1 | `millis()` overflow w timeout | Wzorzec `(now - start) >= duration` |
| 2 | `simpleStop()` nie aktualizuje state/position | Usunięcie `simpleStop()`, zawsze pełny `stopMotor()` |
| 3 | Brak timeout dla ręcznego sterowania | Safety timeout = upTime/downTime |
| 4 | Odwrócone mapowanie przycisków | UP button → goUP, DOWN button → goDOWN |
| 5 | Brak czyszczenia rejestru komendy | Auto-zerowanie HR0 po przetworzeniu |
| 6 | Auto-move zawsze używa UP_TIME | Użycie odpowiedniego czasu wg kierunku |
| 7 | Brak arbitrażu Modbus ↔ przyciski | Przyciski mają wyższy priorytet (override) |
| 8 | `INPUT` bez pull-up | `INPUT_PULLUP` |
| 9 | Podwójna inicjalizacja Serial | Tylko `slave.config()` |
| 10 | Nieużywane zmienne | Usunięte |
| 11 | `getACSReadings()` martwy kod | Zintegrowane jako `updateACS()` w pętli głównej |

---

## Nowa mapa rejestrów Modbus (Holding Registers)

| HR | Kierunek | Nazwa | Wartości | Opis |
|----|----------|-------|----------|------|
| 0 | **Zapis** | Komenda | 0=STOP, 2=OPEN, 4=CLOSE, 6=CALIBRATE | Rozkaz sterujący |
| 1 | **Odczyt** | Stan | 0–5 (patrz tabela stanów) | Aktualny stan urządzenia |
| 2 | **Zapis** | Pozycja docelowa | 0–100 | Zmiana wartości wyzwala pozycjonowanie |
| 3 | **Odczyt** | Pozycja aktualna | 0–100 | Estymacja pozycji w czasie rzeczywistym |
| 4 | **Zapis** | Adres Modbus | 50–80 | Nowy adres → zapis EEPROM → auto-restart |
| 5 | **Odczyt** | UP_TIME | jednostki 100ms | Skalibrowany czas otwarcia |
| 6 | **Odczyt** | DOWN_TIME | jednostki 100ms | Skalibrowany czas zamknięcia |

### Stany urządzenia (HR 1)

| Wartość | Stan | Opis |
|---------|------|------|
| 0 | STOP | Motor zatrzymany |
| 1 | OPENING | Motor jedzie w górę |
| 2 | OPEN | Roleta w pełni otwarta |
| 3 | CLOSING | Motor jedzie w dół |
| 4 | CLOSED | Roleta w pełni zamknięta |
| 5 | CALIBRATING | Trwa kalibracja |

### Detekcja zmian w rejestrach

- **HR 0 (Komenda)**: reaguje na zmianę wartości, po przetworzeniu rejestr jest automatycznie zerowany
- **HR 2 (Pozycja docelowa)**: reaguje na zmianę wartości (aby wysłać tę samą pozycję ponownie, master musi wpisać inną wartość, np. 255, potem żądaną)
- **HR 4 (Adres)**: reaguje na zmianę wartości; po zapisie poprawnego adresu urządzenie restartuje się automatycznie

---

## Layout EEPROM

| Offset | Rozmiar | Zawartość | Domyślnie |
|--------|---------|-----------|-----------|
| 0 | 1 B | Magic byte | 0xA5 = dane poprawne |
| 1 | 1 B | Adres Modbus (uint8_t) | 50 |
| 2–5 | 4 B | UP_TIME w ms (uint32_t) | 30000 |
| 6–9 | 4 B | DOWN_TIME w ms (uint32_t) | 30000 |

**Walidacja przy odczycie:**
- Magic ≠ 0xA5 → użyj wartości domyślnych
- Adres poza 50–80 → adres = 50
- Czasy poza 1000–120000 ms → czas = 30000

> [!IMPORTANT]
> Adres Modbus jest ładowany z EEPROM **przed** konstrukcją obiektu `ModbusSerial` (w fazie inicjalizacji zmiennych globalnych), używając niskopoziomowej funkcji `eeprom_read_byte()` z `<avr/eeprom.h>`. Dzięki temu nie potrzeba dynamicznej alokacji ani modyfikacji biblioteki.

---

## Detekcja krańcówek — ACS712

### Założenia
- Rolety mają wyłączniki krańcowe (mechaniczne)
- Po osiągnięciu krańca obwód motoru jest rozwarty → prąd = 0
- Czujniki ACS712-05B na pinach A4 (kanał UP) i A5 (kanał DOWN)

### Algorytm

```
Każda iteracja loop():
  1. Odczytaj ADC z odpowiedniego kanału (A4 lub A5)
  2. Aktualizuj wartości min/max (śledzenie peak-to-peak fali AC 50Hz)
  3. Co 200ms: oblicz peak-to-peak = max - min
  4. Jeśli peak-to-peak < ACS_THRESHOLD → brak prądu → krańcówka
```

### Parametry

| Stała | Wartość | Opis |
|-------|---------|------|
| `ACS_THRESHOLD` | 15 (ADC units) | Próg peak-to-peak poniżej którego uznajemy brak prądu. **Do dobrania doświadczalnie.** |
| `ACS_WINDOW_MS` | 200 ms | Okres zbierania próbek (min. 4 okresy 50Hz) |
| `ACS_STARTUP_DELAY_MS` | 1000 ms | Czas ignorowania ACS po starcie motoru |

### Reakcja na wykrycie krańcówki

- Podczas **normalnego ruchu**: stop, ustaw pozycję na 0 (otwarta) lub 100 (zamknięta)
- Podczas **pozycjonowania**: stop, skoryguj pozycję (0 lub 100 — nadpisuje estymację)
- Podczas **kalibracji**: przejdź do następnej fazy

---

## Procedura kalibracji

Wyzwalana przez zapis wartości `6` do HR 0.

```
Faza 1: GOING_UP_INIT
  → Motor UP, czekaj na krańcówkę (ACS = 0)
  
Faza 2: PAUSE_1 (500 ms, nieblokujące)
  → Motor zatrzymany, odczekaj przed zmianą kierunku

Faza 3: GOING_DOWN
  → Motor DOWN, start stopera
  → Czekaj na krańcówkę → zapisz DOWN_TIME

Faza 4: PAUSE_2 (500 ms, nieblokujące)
  → Motor zatrzymany, odczekaj

Faza 5: GOING_UP_FINAL
  → Motor UP, start stopera
  → Czekaj na krańcówkę → zapisz UP_TIME

Zakończenie:
  → Zapisz UP_TIME i DOWN_TIME do EEPROM
  → Ustaw state = OPEN, pozycja = 0
```

> [!WARNING]
> Pauzy 500ms realizowane **nieblokująco** (timer w pętli głównej), aby nie blokować WDT i komunikacji Modbus. Każda faza kalibracji ma safety timeout = 3× domyślny czas (90s), po którym kalibracja jest przerywana.

---

## Pozycjonowanie do zadanej pozycji

Wyzwalane przez zmianę wartości w HR 2 (0–100).

### Algorytm

```
1. Oblicz różnicę: diff = |target - currentPosition|
2. Jeśli diff == 0 → nic nie rób
3. Kierunek: target < current → UP, target > current → DOWN
4. Czas jazdy: moveTime = diff × totalTime / 100
5. Start motoru, ustaw timer
6. Stop gdy: timer upłynie LUB ACS wykryje krańcówkę
```

### Estymacja pozycji w czasie rzeczywistym

Pozycja jest obliczana dynamicznie podczas ruchu (nie tylko przy zatrzymaniu), aby rejestr HR 3 zawsze pokazywał aktualną estymację:

```cpp
int16_t getEstimatedPosition() {
    if (state != OPENING && state != CLOSING) return currentPosition;
    elapsed = millis() - moveStartTime;
    percentMoved = min(elapsed * 100 / totalTime, 100);
    if (direction == UP) return max(currentPosition - percentMoved, 0);
    else return min(currentPosition + percentMoved, 100);
}
```

---

## Zmiana adresu Modbus

1. Master zapisuje nowy adres (50–80) do HR 4
2. Urządzenie waliduje zakres
3. Jeśli poprawny i inny niż aktualny:
   - Zatrzymaj motor (jeśli pracuje)
   - Zapisz nowy adres do EEPROM
   - Wykonaj **software reset** (WDT trick: `wdt_enable(WDTO_15MS); while(1){}`)
4. Urządzenie uruchamia się z nowym adresem

> [!IMPORTANT]
> Po zmianie adresu urządzenie **natychmiast się restartuje**. Master musi przełączyć się na nowy adres aby kontynuować komunikację. Pozycja rolety po restarcie jest nieznana (0).

---

## Logika przycisków (poprawiona)

### Mapowanie (naprawione)
- Przycisk UP (A3) → motor jedzie **w górę** (otwieranie)
- Przycisk DOWN (A2) → motor jedzie **w dół** (zamykanie)

### Tryby

| Warunek | Akcja |
|---------|-------|
| Jeden przycisk wciśnięty, motor stoi | Start w odpowiednim kierunku (hold-to-run) |
| Jeden przycisk wciśnięty, motor pracuje (auto) | STOP (przerwanie autoMove) |
| Oba przyciski | Pełny przejazd w ostatnim kierunku (autoMove) |
| Oba zwolnione | STOP (jeśli tryb ręczny) |

### Safety timeout
- Ręczne sterowanie (hold-to-run) ma timeout = upTime/downTime
- Po przekroczeniu → automatyczny stop

### Priorytet nad Modbus
- Wciśnięcie przycisku **nadpisuje** aktualną komendę Modbus
- Podczas kalibracji przyciski **przerywają** kalibrację (safety)

---

## Struktura kodu

### [NEW] [roller_shutter_2026.ino](file:///c:/xampp/htdocs/work/arduino/roller_shutter_2026/roller_shutter_2026.ino)

Plik zostanie **nadpisany** nowym programem. Struktura:

```
1. Includy i definicje stałych
2. EEPROM: layout, ładowanie, zapisywanie
3. Zmienne globalne
4. Inicjalizacja Modbus (adres z EEPROM)
5. setup(): piny, przyciski, Modbus, WDT
6. Sterowanie przekaźnikami: relayStop/Up/Down
7. Sterowanie motorem: startMotor, stopMotor
8. Obliczanie pozycji: getEstimatedPosition
9. ACS712: updateACS, isEndStopReached
10. Kalibracja: startCalibration, updateCalibration
11. Obsługa Modbus: processModbus
12. Obsługa przycisków: processButtons
13. loop(): WDT, Modbus, ACS, kalibracja, przyciski, aktualizacja rejestrów
```

---

## Weryfikacja

### Kompilacja
- Kompilacja dla Arduino Nano (ATmega328P) — sprawdzenie zużycia Flash (<32KB) i RAM (<2KB)

### Estymacja zużycia pamięci
- **Flash**: ~15–18 KB (ModbusSerial ~8KB, Bounce2 ~2KB, logika ~5–8KB) — mieści się w 32KB
- **RAM**: ~300–500 B zmienne + ~200 B bufory Serial/Modbus — mieści się w 2KB
- **EEPROM**: 10 B z 1024 B dostępnych
