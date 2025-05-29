#include <Wire.h>
#include <DS3231.h>
#include <SoftwareSerial.h>
#include <Stepper.h>
#include <DHT.h>

#define DHTPIN 6          
#define DHTTYPE DHT22     
#define LDRPIN A0          
#define BT_RX 4           
#define BT_TX 5           


SoftwareSerial btSerial(BT_RX, BT_TX); 
DS3231 rtc;
DHT dht(DHTPIN, DHTTYPE);
Stepper motor(4186, 8, 9, 10, 11); 


bool h12, pm;
bool alarmaActivata = false;
int oraAlarma = -1;
int minutAlarma = -1;
bool jaluzeleDeschise = false;

const int rpm = 14;
const int pragLuminozitate = 150;
const float pragTemp = 30.0; 


void deschideJaluzele() {
  if (!jaluzeleDeschise) {
    motor.setSpeed(rpm);
    motor.step(4186);
    jaluzeleDeschise = true;
    btSerial.println("Jaluzelele s-au deschis.");
  } else {
    btSerial.println("Jaluzelele sunt deja deschise.");
  }
}

void inchideJaluzele() {
  if (jaluzeleDeschise) {
    motor.setSpeed(rpm);
    motor.step(-4186);
    jaluzeleDeschise = false;
    btSerial.println("Jaluzelele s-au inchis.");
  } else {
    btSerial.println("Jaluzelele sunt deja inchise.");
  }
}

void trimiteStatus() {
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  int lum = analogRead(LDRPIN);  
  byte h = rtc.getHour(h12, pm);
  byte m = rtc.getMinute();
  byte s = rtc.getSecond();

  btSerial.print("\nStatus la ora:");
  btSerial.print(h);
  btSerial.print(":");
  btSerial.print(m);
  btSerial.print(":");
  btSerial.print(s);
  btSerial.print("\n");

  btSerial.print("Temperatura: ");
  btSerial.println(temp);
  btSerial.print("\nUmiditate: ");
  btSerial.println(umid);
  btSerial.print("\nLuminozitate (digital): ");
  btSerial.println(lum);
}


void setup() {
  Wire.begin();
  motor.setSpeed(rpm);
  dht.begin();

  pinMode(LDRPIN, INPUT);

  Serial.begin(9600);
  btSerial.begin(9600);

  byte h = rtc.getHour(h12, pm);
  byte m = rtc.getMinute();
  byte s = rtc.getSecond();

  btSerial.print("Ora curenta RTC: ");
  btSerial.print(h); btSerial.print(":");
  btSerial.print(m); btSerial.print(":");
  btSerial.println(s);

  btSerial.println("Sistem pornit. Trimite 'open', 'close', 'alarm:HH:MM', 'status'");
}

void loop() {
  if (btSerial.available()) {
    String comanda = btSerial.readStringUntil('\n');
    comanda.trim();

    if (comanda == "open") {
      deschideJaluzele();
    } else if (comanda == "close") {
      inchideJaluzele();
    } else if (comanda.startsWith("alarm:")) {
      int sep1 = comanda.indexOf(':');
      int sep2 = comanda.indexOf(':', sep1 + 1);
      if (sep1 != -1 && sep2 != -1) {
        oraAlarma = comanda.substring(sep1 + 1, sep2).toInt();
        minutAlarma = comanda.substring(sep2 + 1).toInt();
        alarmaActivata = false;
        btSerial.print("Alarma setata la: ");
        btSerial.print(oraAlarma); btSerial.print(":"); btSerial.println(minutAlarma);
      }
    } else if (comanda == "status") {
      trimiteStatus();
    } else {
      btSerial.println("Comanda necunoscuta.");
    }
  }

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck >= 1000) {
    lastCheck = millis();

    byte ora = rtc.getHour(h12, pm);
    byte minut = rtc.getMinute();

    float temp = dht.readTemperature();
    int lum = analogRead(LDRPIN);

    if ((temp > pragTemp || lum < pragLuminozitate) && jaluzeleDeschise) {
      btSerial.println("Conditii depasite, se inchid jaluzelele automat.");
      inchideJaluzele();
    }

    if (!alarmaActivata && ora == oraAlarma && minut == minutAlarma) {
      alarmaActivata = true;
      btSerial.println("Alarma activata, se deschid jaluzelele");
      deschideJaluzele();
    }
  }
}
