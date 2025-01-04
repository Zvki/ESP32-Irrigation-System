#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

// Ustawienia wyświetlacza OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define I2C_ADDRESS   0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Ustawienia czujnika DHT11
#define DHTPIN 16       // GPIO16
#define DHTTYPE DHT11   // Typ czujnika: DHT11

// Ustawienia czujnika SEN0193

#define SENS0193PIN 4

const char* ssid = "TELPOL-10686";        // Nazwa sieci Wi-Fi
const char* password = "u4433nzmhy";  // Hasło sieci Wi-Fi

WebServer server(80); // Tworzy serwer na porcie 80

const int AirSENS0193 = 3000;  // Wartość dla suchego powietrza
const int WaterSENS0193 = 1500; // Wartość dla pełnego zanurzenia

int soilMoistureValue = 0;
int soilMoisturePercent = 0;

float temperature = 0.0f;
float humidity = 0.0f;

DHT dht(DHTPIN, DHTTYPE);

void DisplayMsg(String msg, int time, int size){
  display.clearDisplay();
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
  delay(time);

}

void setup() {
    Serial.begin(115200);

    // Inicjalizacja magistrali I2C (GPIO8 jako SDA, GPIO7 jako SCL)
    Wire.begin(8, 7);

        // Inicjalizacja wyświetlacza OLED z SSD1306_SWITCHCAPVCC
    if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
        Serial.println(F("Nie można zainicjalizować wyświetlacza OLED"));
        while (true); // Zatrzymaj program w przypadku błędu
    }

    WiFiSetup();

    ServerSetup();

    // Inicjalizacja czujnika DHT
    dht.begin();

    // Wyświetlenie komunikatu startowego
    DisplayMsg("Hello!", 2000, 2);
}

void WiFiSetup(){

   WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        DisplayMsg("Lacze.", 300, 2);
        DisplayMsg("Lacze..", 300, 2);
        DisplayMsg("Lacze...", 300, 2);
    }
    DisplayMsg("Polaczono!", 2000, 2);
    IPAddress ip = WiFi.localIP();
    String ipString = ip.toString();
    DisplayMsg(ipString, 10000, 1); 

}

void ServerSetup(){
    server.on("/", HTTP_GET, []() {
        String* readings = getReadings(); // Pobranie wskaźnika do tablicy odczytów
        // Tworzenie strony HTML z dynamicznymi odczytami
        String page = "<!DOCTYPE html>"
                      "<html lang=\"en\">"
                      "<head>"
                      "<meta charset=\"UTF-8\">"
                      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                      "<title>Monitor ESP32</title>"
                      "<style>"
                      "body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }"
                      "h1 { color: #333; }"
                      ".container { max-width: 600px; margin: 50px auto; padding: 20px; background: #fff; box-shadow: 0 2px 5px rgba(0,0,0,0.3); }"
                      ".reading { font-size: 1.5em; color: #007BFF; margin: 10px 0; }"
                      "</style>"
                      "</head>"
                      "<body>"
                      "<div class=\"container\">"
                      "<h1>Witaj w monitorze ESP32</h1>"
                      "<h2>Temperature: </h2>"
                      "<p class=\"reading\">" + readings[0] + " C</p>"
                      "<h2>Humidity: </h2>"
                      "<p class=\"reading\">" + readings[1] + " %</p>"
                      "<h2>SoilMoisture: </h2>"
                      "<p class=\"reading\">" + readings[2] + " %</p>"
                      "</div>"
                      "</body>"
                      "</html>";

        // Wysyłanie strony HTML do klienta
        server.send(200, "text/html", page);
    });

    server.begin();
}

void SoilHumSensor() {
    // Odczyt z czujnika wilgotności gleby
    soilMoistureValue = analogRead(SENS0193PIN);

    // Sprawdzenie, czy czujnik jest podłączony i w zakresie
    if (soilMoistureValue < 1300 || soilMoistureValue > 3300) {
        DisplayMsg("Brak SENS0193!", 2000, 1);
        return; // Opuszczenie funkcji, jeśli czujnik jest poza zakresem
    }

    // Obliczanie wilgotności w procentach
    soilMoisturePercent = map(soilMoistureValue, WaterSENS0193, AirSENS0193, 100, 0);

    // Wyświetlenie wilgotności na OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("SENS0193 Odczyt:"));
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(soilMoisturePercent);
    display.print(F(" %"));
    display.display();
    delay(5000);
}

void TempHumSensor(){
    temperature = dht.readTemperature(); // Odczyt temperatury w °C
    humidity = dht.readHumidity();      // Odczyt wilgotności w %

    // Sprawdzenie poprawności odczytów
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println(F("Nie można odczytać danych z DHT11"));
    } else {
        // Wyświetlenie danych na OLED
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println(F("DHT11 Odczyt:"));
        display.setTextSize(2);
        display.setCursor(0, 16);
        display.print(temperature);
        display.print(F(" C"));
        display.setCursor(0, 40);
        display.print(humidity);
        display.print(F(" %"));
        display.display();
    }
}

String* getReadings(){
  static String readings[3];
  readings[0] = String(temperature);
  readings[1] = String(humidity);
  readings[2] = String(soilMoisturePercent);  
  return readings;
}

void loop() {
    server.handleClient();
    TempHumSensor();  // Odczyt danych z DHT11
    delay(5000);  // Opóźnienie dla DHT11
    SoilHumSensor();  // Odczyt wilgotności gleby
}
