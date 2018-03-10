#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FS.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;
File audioFile;
#define CHUNK_SIZE 10000
uint8_t audioBuffer[CHUNK_SIZE];

char* ssid = "wifi ssid";
char* pass = "wifi password";
//char* host = "192.168.43.219";    // IP address of Upload Server
char* host = "your server ip or name";    // IP address of Upload Server
String port = "3000";           // Port of Upload Server
String uploadPath = "/upload";  // Path of Upload URL
int currentstate = 0, previousstate = 0, recording = 0;


WiFiClient client;

bool reconnectedWithFiles = false;

void eventWiFi(WiFiEvent_t event) {
  if(event == WIFI_EVENT_STAMODE_CONNECTED) {
    Dir d = SPIFFS.openDir("/");

    reconnectedWithFiles = d.next();

  }
}
void setup() {
  Serial.begin(9600);
  delay(200);
  pinMode(D7, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

//  WiFi.setAutoConnect ( false );                                             // Autoconnect to last known Wifi on startup
  WiFi.setAutoReconnect ( true );
  WiFi.onEvent(eventWiFi);                                                    // Handle WiFi event
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to WiFi: ");
  Serial.print(ssid); Serial.print("  "); Serial.println(pass);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Beginning WiFi");
    WiFi.begin(ssid, pass);
  }

  int ctr = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi Status:"); Serial.println(WiFi.status());
    delay(500); ctr++;
    Serial.print(".");
    if(ctr > 20) break;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());

  } else {
    Serial.println("\nNo WiFi, proceeding...");
  }

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  SPIFFS.begin();
  delay(300);
  Serial.println("\nWaiting...");
}

void loop()
{
   previousstate = currentstate;
   delay(1000);
   currentstate = digitalRead(D7);
//   Serial.printf("Current: %d\n", currentstate);
//   Serial.print(" "); Serial.print(previousstate); Serial.print(currentstate); Serial.print(recording); Serial.println(" ");
   if((previousstate == 1) && (currentstate == 0) && (recording == 0)) {
     recording = 1;
     sampleAndSave(6000);
     delay(500);
     sendIfWiFi();
     recording = 0;
     Serial.println("Waiting...");
   }

   if(reconnectedWithFiles && recording == 0) {
    Serial.println("Reconnected...now sending files");
    sendFiles();
    reconnectedWithFiles = false;
   }
}


void sendIfWiFi() {

  Serial.print("WiFi Status:"); Serial.println(WiFi.status());
  if(WiFi.status() == WL_CONNECTED) {
    sendFiles();
  }
}


int count = 0;
void sendFiles() {
    Serial.println("Sending files...");
    String NL = "\r\n";
    char buf[1024];
    byte respCode, thisByte;
    client.setNoDelay(true);
    bool success = false;
    do{   // Loop to retry connecting to server 3 times
      if (client.connect(host, 3000))
      {
        Serial.println("Connected to upload server");
        success = true;
        String fname, endb, req="", hdr="";
        SPIFFS.begin();
        Dir dir = SPIFFS.openDir("/");
        long content_length = 0;
        long boundary = millis();
        endb = NL + "--------------------------"+ String(boundary) +"--" + NL;
        while (dir.next()) {  // Begin loop to calculate content_length (sum of all file data + all hdrs)
          fname = dir.fileName();
          File spiFile = dir.openFile("r");
          content_length += spiFile.size();
          hdr = "";
          hdr +="--------------------------"+ String(boundary) +"\r\n";
          hdr +="Content-Disposition: form-data; name=\"a_"+ fname.substring(1) +"\"; filename=\""+ fname +"\"" + NL;
          hdr +="Content-Type: application/octet-stream" + NL + NL;
          content_length += hdr.length();
          spiFile.close();
        }
        content_length += endb.length() + 10;  // plus end boundary size (only once)

        Serial.println("\n--- Begin Request ---");
        req +="POST "+ uploadPath +" HTTP/1.1" + NL;
        req +="Host: "+ String(host) +":" + port + NL;
        req +="User-Agent: curl/7.47.0" + NL;
        req +="Accept: */*" + NL;
        req +="Content-Length: "+ String(content_length) + NL;
        req +="Expect: 100-continue" + NL;
        req +="Content-Transfer-Encoding: binary" + NL;
        req +="Content-Type: multipart/form-data; boundary=------------------------"+ String(boundary) + NL + NL;
        client.print(req);
        Serial.print(req);
        dir = SPIFFS.openDir("/");
        while (dir.next()) {  // Begin loop for writing file data to client
          fname = dir.fileName();
          File spiFile = dir.openFile("r");
          int fsize = spiFile.size();
          hdr = "";
          hdr +="--------------------------"+ String(boundary) +"\r\n";
          hdr +="Content-Disposition: form-data; name=\"a_"+ fname.substring(1) +"\"; filename=\""+ fname +"\"" + NL;
          hdr +="Content-Type: application/octet-stream" + NL + NL;
          client.print(hdr);
          Serial.print(hdr);
          Serial.print("DATA: ");
          while(fsize > 0) {
            size_t len = std::min((int)(sizeof(buf) - 1), fsize);
            spiFile.read((uint8_t *)buf, len);
            size_t ln = client.write((const char*)buf, len);
            Serial.print(".");
            fsize -= len;
          }
//          spiFile.close();    // Not sure if closing allows removing later
          client.flush();
          delay(100);
          Serial.print("\nDeleting file"); Serial.println(fname);
          SPIFFS.remove(fname);
        }  // End loop
        client.print(endb);     // End boundary only once, after all files are written to client
        Serial.println(endb);
        Serial.println("--- End Request ---");

        Serial.println("--- Begin Response ---");
        delay(100);
        while(!client.available()) delay(1);
        respCode = client.peek();
        while(client.available())
        {
          thisByte = client.read();
          Serial.write(thisByte);
        }
        Serial.println("\n--- End Response ---");


        client.flush();

        delay(1000);
        client.stop();
        return;
      }
      else {
        Serial.println("Unable to connect to upload server");
        count++;
        Serial.printf("Retrying... %d\n", count);
      }

    }
    while(count < 3);
    if(!success) Serial.println("Giving up after 3 retries. Will try next time recording happens.");
    count = 0;
}
int buf_count = 0;

void create() {  // Creates
  char fileName[24];
  DateTime now = rtc.now();
  sprintf(fileName, "/%d_%d_%d_%d_%d_%d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  audioFile = SPIFFS.open(fileName, "w");
  if(!audioFile) {
    Serial.println("Error creating file");
  }

  Serial.println("Recording...");
}

void sampleAndSave(int delay) {
  create();
//  digitalWrite(LED_BUILTIN, LOW);
  unsigned long started = millis();
  while( (millis() - started) < delay ){
    uint8_t value = 25 + analogRead(A0)/4;
    audioBuffer[buf_count++] = value;
    if(buf_count == CHUNK_SIZE) {
      size_t len = audioFile.write((uint8_t*)audioBuffer, CHUNK_SIZE);

      if(len == 0) {
        Serial.println("didn't write");
      }
      //reset buf_count AND clear buffer
      buf_count = 0;
      memset(audioBuffer, -1, CHUNK_SIZE);
    }

  }

  if(audioFile) {
    audioFile.close();
  }
//  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Done recording.");
}
