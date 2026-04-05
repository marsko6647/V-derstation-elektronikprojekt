#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <Adafruit_BMP085.h>

#define ONE_WIRE_BUS 8 // digital pin 8 för temp sensorer
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// konstanter osv som har med vindsensorn att göra
const int sensorPin = A1;
const float Vref = 5.0; // 5 V Arduino
const int ADCmax = 1023;
const float V_zero = 0.41; // utspänning vid 0 m/s
const float V_max = 2.0;    
const float WS_max = 32.4;  
const float K_wind = WS_max / (V_max - V_zero); // denna kvot blir ungefär 20.25 m/s per volt
const float deadband_ms = 0.30; // all vindhastighet under denna behandlas som 0
const float alpha = 0.3; 
float wind_filtered = 0.0;
bool firstRun = true;

// lcd skärm och pins till denna
LiquidCrystal lcd(12, 11, 3, 4, 5, 6); //RS, E D4, D5, D6, D7 GRÖN BRUN GUL BLÅ i stigande ordning på arduinon

// regnsensor
bool isRaining = false;
int rainDigitalPin = 2; //Digital Regn-pin

// trycksensor
Adafruit_BMP085 bmp;

void setup() {
  pinMode(A0, INPUT_PULLUP);
  pinMode(rainDigitalPin, INPUT);
  pinMode(7, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(A2, OUTPUT);

  Serial.begin(9600);
  sensors.begin();
  lcd.begin(20, 2); //tecken, rader
  bmp.begin();
}

void loop() {
  // --- TEMPERATUR ---
  sensors.requestTemperatures();
  float tempInside = sensors.getTempCByIndex(0);
  float tempOutside = sensors.getTempCByIndex(1);
  // printa temp lcd
  lcd.setCursor(0, 0);
  lcd.print("     ");    
  lcd.setCursor(0, 0); // detta rensar inför nästa mättillfälle

  if (digitalRead(A0) == HIGH) {
    lcd.print(tempInside, 1);
    Serial.print(tempInside, 1); ///test
    lcd.print((char)223);
    lcd.print("C");
    Serial.print("C"); ///test
  } else {
    lcd.print(tempOutside, 1);
    Serial.print(tempOutside, 1); //test
    lcd.print((char)223);
    lcd.print("C");
    Serial.print("C"); ///test
  }

  // varning för kyla
  if (tempInside < 0) {
    digitalWrite(13, HIGH); // Tänd LED
  } else {
    digitalWrite(13, LOW);  // Släck LED
  }

  // varning för värmebölja
  if (tempInside > 27) {
    digitalWrite(A2, HIGH); // Tänd LED
  } else {
    digitalWrite(A2, LOW);  // Släck LED
  }

  // --- REGN ---
  isRaining = !(digitalRead(rainDigitalPin));  

  lcd.setCursor(0, 1);
  lcd.print("Rain:"); 
  Serial.print(" Rain:"); //test

  if (isRaining) { // om sant print YES
    lcd.print("YES ");
    Serial.print(" YES   "); //test
  } else {
    lcd.print("NO ");
    Serial.print(" NO   "); //test
  }

  // --- TRYCK ---
  float pressure_hPa = bmp.readPressure() / 100.0;

  lcd.setCursor(9, 0);
  lcd.print(pressure_hPa);
  Serial.print(pressure_hPa); //test
  lcd.println(" hPa         ");
  Serial.print(" hPa    "); //test

  // --- VIND ---
  const int N = 30; // större N jämnare data
  long sum = 0; // börja summa på 0
  
  for (int i = 0; i < N; i++) {
    sum += analogRead(sensorPin);
    delay(5);
  }
  float raw = sum / (float)N;

  // spänning vid A1
  float V_sensor = raw * (Vref / ADCmax);

  // gör om spänning till vindhastighet
  float wind_ms = 0.0;

  if (V_sensor > V_zero) {
    wind_ms = (V_sensor - V_zero) * K_wind;
    if (wind_ms > WS_max) {
      wind_ms = WS_max;
    }
  } else {
    wind_ms = 0.0;
  }

  // små vindar behandlas som 0
  if (wind_ms < deadband_ms) {
    wind_ms = 0.0;
  }

  // utjämning av data 
  if (firstRun) {
    wind_filtered = wind_ms;
    firstRun = false;
  } else {
    wind_filtered = alpha * wind_ms + (1.0 - alpha) * wind_filtered;
  }
  
  // print vind lcd
  lcd.setCursor(9, 1);
  lcd.print(wind_filtered, 1);
  lcd.print(" m/s");
  
  Serial.print(wind_filtered, 1);
  Serial.println(" m/s");
  Serial.print("\n");
  delay(500);

  // LED to check wind speed
  if (wind_filtered < 1) {
    digitalWrite(7, HIGH);
    digitalWrite(10, LOW);
    digitalWrite(9, LOW);
  }
  else if (wind_filtered >= 1 && wind_filtered < 3) {
    digitalWrite(10, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(9, LOW);
  }
  else {
    digitalWrite(9, HIGH);
    digitalWrite(7, LOW);
    digitalWrite(10, LOW);
  }
  
}
