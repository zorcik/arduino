    #include "DHT.h"          // biblioteka DHT
     
    #define DHTPIN 2          // numer pinu sygnałowego
    #define DHTPIN22 4          // numer pinu sygnałowego
     
    DHT dht;                  // definicja czujnika
    DHT dht22;                  // definicja czujnika
     
    void setup()
    {
      Serial.begin(9600);     // otworzenie portu szeregowego
      dht.setup(DHTPIN);      // inicjalizacja czujnika
      dht22.setup(DHTPIN22);      // inicjalizacja czujnika
    }
     
    void loop()
    {
      // Miniamalne opóźnienie odczytu
      int delay1 = dht.getMinimumSamplingPeriod();
      int delay2 = dht22.getMinimumSamplingPeriod();
      delay(delay1 > delay2 ? delay1 : delay2);
     
      // Odczyt temperatury i wilgotności powietrza
      float t = dht.getTemperature();
      float h = dht.getHumidity();

      float t22 = dht22.getTemperature();
      float h22 = dht22.getHumidity();
     
      // Sprawdzamy poprawność danych
      if (dht.getStatus() || dht22.getStatus())
      {
        // Jeśli nie, wyświetlamy informację o błędzie
        Serial.println("Blad odczytu danych z czujnika");
      } else
      {
        // Jeśli tak, wyświetlamy wyniki pomiaru
        Serial.print("DHT 11: Wilgotnosc: ");
        Serial.print(h);
        Serial.print(" % ");
        Serial.print("Temperatura: ");
        Serial.print(t);
        Serial.print(" *C / ");
        Serial.print(dht.toFahrenheit(t));
        Serial.println(" *F ");
        Serial.print("DHT 22: Wilgotnosc: ");
        Serial.print(h22);
        Serial.print(" % ");
        Serial.print("Temperatura: ");
        Serial.print(t22);
        Serial.print(" *C / ");
        Serial.print(dht22.toFahrenheit(t22));
        Serial.println(" *F ");
      }
    }

