# Roller Shutter Controller v2.0 — Walkthrough

## Co zostało zrobione

Kompletna przebudowa firmware sterownika rolet z ATmega328P. Stary program (346 linii) został zastąpiony nowym (~480 linii) z poprawionymi błędami i 4 nowymi funkcjami.

## Zmienione pliki

### [roller_shutter_2026.ino](file:///c:/xampp/htdocs/work/arduino/roller_shutter_2026/roller_shutter_2026.ino) — kompletnie nowy

Nowy program z modularną strukturą:

| Sekcja | Linie | Opis |
|--------|-------|------|
| Header + defines | 1–128 | Stałe konfiguracyjne, piny, stany, komendy, layout EEPROM |
| EEPROM load (global init) | 130–139 | Ładowanie adresu przed `main()` |
| Zmienne globalne | 141–182 | Wszystkie zmienne stanu urządzenia |
| EEPROM save/load | 184–210 | Funkcje zapisu/odczytu EEPROM |
| Relay control | 216–232 | `relayStop()`, `relayUp()`, `relayDown()` |
| Motor control | 237–297 | `startMotor()`, `stopMotor()`, `stopMotorAtEndStop()` |
| Position estimation | 299–320 | `getEstimatedPosition()` — estymacja w czasie rzeczywistym |
| ACS712 | 322–359 | `updateACS()`, `isEndStopReached()` |
| Calibration | 361–428 | 5-fazowa maszyna stanów (nieblokująca) |
| Modbus processing | 430–502 | Przetwarzanie HR 0, HR 2, HR 4 |
| Button processing | 504–554 | Logika przycisków z priorytetem |
| setup() | 556–586 | Inicjalizacja hardware i Modbus |
| loop() | 588–~480 | 9-krokowa pętla główna |

### [DOCUMENTATION.md](file:///c:/xampp/htdocs/work/arduino/roller_shutter_2026/DOCUMENTATION.md) — zaktualizowane

Pełna dokumentacja techniczna zaktualizowana do v2.0 z nowymi rejestrami, kalibracją, pozycjonowaniem i przykładami sekwencji Modbus.

## Nowe funkcje

### 1. Detekcja krańcówek (ACS712)
- Pomiar peak-to-peak w oknach 200ms, odpowiedni kanał (UP/DOWN) zależnie od kierunku
- Ignorowanie 1s po starcie motoru (transient)
- Próg `ACS_THRESHOLD = 15` (do kalibracji doświadczalnej)
- Krańcówka → korekta pozycji do 0 lub 100

### 2. Kalibracja (HR 0 = 6)
- 5-fazowa procedura: UP→top → pause → DOWN (pomiar) → pause → UP (pomiar)
- Pauzy 500ms realizowane nieblokująco
- Safety timeout 90s/fazę
- Czasy zapisywane do EEPROM

### 3. Adres Modbus (HR 4)
- Zakres 50–80, domyślnie 50
- Zapis → EEPROM → auto-restart
- Ładowany z EEPROM przed `main()` via `eeprom_read_byte()`

### 4. Pozycjonowanie (HR 2)
- Zmiana wartości 0–100 wyzwala ruch proporcjonalny
- Automatyczny dobór kierunku i czasu
- Korekta pozycji przy detekcji krańcówki

## Poprawione błędy

| # | Bug | Poprawka |
|---|-----|---------|
| 1 | `millis()` overflow | Wzorzec `(now - start) >= duration` |
| 2 | `simpleStop()` nie aktualizuje stanu | Usunięte, zawsze `stopMotor()` z obliczaniem pozycji |
| 3 | Brak timeout manualnego sterowania | Safety timeout = upTime/downTime |
| 4 | Odwrócone mapowanie przycisków | UP→goUP, DOWN→goDOWN |
| 5 | Brak czyszczenia HR 0 | Auto-clear + `prevCommand = CMD_STOP` |
| 6 | Auto-move zawsze UP_TIME | `getTimeForDirection()` — odpowiedni czas wg kierunku |
| 7 | Brak arbitrażu Modbus/przyciski | Przyciski po Modbus w loop = wyższy priorytet |
| 8 | INPUT bez pull-up | INPUT_PULLUP |
| 9 | Podwójna inicjalizacja Serial | Tylko `slave.config(9600)` |
| 10 | Nieużywane zmienne | Usunięte |
| 11 | getACSReadings() martwy kod | Zintegrowane jako updateACS() |
| 12 | Brak ochrony WDT boot-loop | `.init3` section: `MCUSR=0; wdt_disable()` |

## Weryfikacja

- **Kompilacja**: Wymaga Arduino IDE z bibliotekami ModbusSerial i Bounce2 — do zweryfikowania przez użytkownika
- **Estymacja pamięci**: Flash ~15–18 KB / 32 KB, RAM ~500–600 B / 2048 B — powinno się zmieścić
