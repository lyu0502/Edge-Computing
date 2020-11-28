/* This code works with MAX30102 + 128x32 OLED i2c + Buzzer and Arduino UNO
 * It's displays the Average BPM on the screen, with an animation and a buzzer sound
 * everytime a heart pulse is detected
 * It's a modified version of the HeartRate library example
 * Refer to www.surtrtech.com for more details or SurtrTech YouTube channel
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include<AsyncTCP.h>
#include<ESPAsyncWebServer.h>

AsyncWebServer server(80);

const char* ssidAP = "ESP32-Access-Point";
const char* passwordAP = "123456789";
#define CHAN_AP 2

//const char* ssid = "ORBI12";
//const char* password = "coolmango116";

const char* ssid = "Knight";
const char* password = "2gEDsvGZ5Fxx7ACa8zlN";

const char* PARAM_INPUT_1 = "patient_id";
const char* PARAM_INPUT_2 = "patient_name";
const char* PARAM_INPUT_3 = "patient_display";
const char* PARAM_INPUT_4 = "deviceId";





//Your Domain name with URL path or IP address with path
const char* serverName = "http://iomt-uci.ngrok.io/patient-connect?edgeId=3";

// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 20000;
String sensorReadings;

#include <Adafruit_GFX.h>        //OLED libraries
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "MAX30105.h"           //MAX3010x library
#include "heartRate.h"          //Heart rate calculating algorithm

#include <esp_now.h> // ESP_Now send data
uint8_t broadcastAddress[] = {0xA4, 0xCF, 0x12, 0x9E, 0xA7, 0xF8};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    char patient_id[30]; // must be unique for each sender board
    char device_id[5];
    char patient_name[50];
    int bpm;
} struct_message;

//Create a struct_message called myData
struct_message myData;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;


int buzzer_init_time = millis();
int buzzer_time_space = 6000;
int buzzer_st;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)

static const unsigned char PROGMEM logo2_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,              //Logo2 and Logo3 are two bmp pictures that display on the OLED if called
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00,  };

static const unsigned char PROGMEM logo3_bmp[] =
{ 0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00  };


int active_mode = 0;


char patient_display[10] = "Null";
int isCalling = 0;
char patient_id[30]; // must be unique for each sender board
char device_id[5];
char patient_name[50];

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    input1: <input type="text" name="patient_id">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    input2: <input type="text" name="patient_name">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    input3: <input type="text" name="patient_display">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    input4: <input type="text" name="deviceId">
  </form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}



void setup() {  
  ledcSetup(0,1E5,12);
  ledcAttachPin(33,0);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Start the OLED display
  display.display();
  delay(3000);
  Serial.begin(115200);
  Serial.println("Initializing...");
  // Initialize sensor
  particleSensor.begin(Wire, I2C_SPEED_FAST); //Use default I2C port, 400kHz speed
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running


  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");

  //Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
    }
     else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = PARAM_INPUT_4;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();

  
  

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  WiFi.softAP(ssidAP, passwordAP, CHAN_AP, true);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }else{
    Serial.println("ESP_NOW success");
  }
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = CHAN_AP;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}




void http_get_request(){
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
              
      sensorReadings = httpGETRequest(serverName);
      JSONVar myObject = JSON.parse(sensorReadings);
      Serial.println(sensorReadings);
    
      active_mode =myObject["isConnected"];

      if( active_mode == 1){
        const char* local_patient_id = myObject["patient_id"];
        const char* local_patient_name = myObject["patient_name"];
        const char* local_patient_display = myObject["patient_display"];
        const char* local_device_id = myObject["deviceId"];

        isCalling = myObject["isCalling"];
        strcpy(patient_id,local_patient_id);
        strcpy(patient_name,local_patient_name);
        strcpy(patient_display,local_patient_display);
        strcpy(device_id,local_device_id);      
      }else{
        Serial.println("Not connected");
      }
      
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(serverName);
  String authToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOiI1ZjI2NTU3YTk1NzkyMjBmOTcwMmJhYzgiLCJpYXQiOjE1OTY0NzYxMTB9.navlMh645_uijvQr152mnRiZP_brLQvJ48qBuVm6rdU";
  http.addHeader("Authorization","Bearer "+authToken);
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
  }
  // Free resources
  http.end();

  return payload;
}

void esp_now_send_data(){

        
        
//        Serial.print("ESP-NOW patient_id: ");
//        Serial.println(patient_id);
//        Serial.print("ESP-NOW patient_name: ");
//        Serial.println(patient_name);
//        Serial.print("ESP-NOW patient_display: ");
//        Serial.println(patient_display);
        Serial.print("ESP-NOW isCalling: ");
        Serial.println(isCalling);
//        Serial.print("ESP-NOW device_id: ");
//        Serial.println(device_id);
        
        strcpy(myData.patient_id,patient_id);
        strcpy(myData.device_id,device_id); 
        strcpy(myData.patient_name,patient_name);   
        myData.bpm = beatAvg;
        
        // Send message via ESP-NOW
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
         
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }
}


void get_bpm_and_show(){
  long irValue = particleSensor.getIR();    //Reading the IR value it will permit us to know if there's a finger on the sensor or not
                                           //Also detecting a heartbeat
  if(irValue > 7000){                                           //If a finger is detected
        
        if(isCalling == 0){
            display.clearDisplay();   
            display.stopscroll();//Clear the display
            display.drawBitmap(5, 5, logo2_bmp, 24, 21, WHITE);       //Draw the first bmp picture (little heart)
            display.setTextSize(2);                                   //Near it display the average BPM you can display the BPM if you want
            display.setTextColor(WHITE); 
            display.setCursor(50,0);                
            display.println("BPM");             
            display.setCursor(50,18);                
            display.println(beatAvg); 
            display.display();
        }
        
        
      if (checkForBeat(irValue) == true)                        //If a heart beat is detected
      {
        esp_now_send_data();
        if(isCalling == 0){
          display.clearDisplay();   
          display.stopscroll();//Clear the display
          display.clearDisplay();  
          display.stopscroll();//Clear the display
          display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE);    //Draw the second picture (bigger heart)
          display.setTextSize(2);                                //And still displays the average BPM
          display.setTextColor(WHITE);             
          display.setCursor(50,0);                
          display.println("BPM");             
          display.setCursor(50,18);                
          display.println(beatAvg); 
          display.display();
          delay(100);
        }
        
        
        
        long delta = millis() - lastBeat;                   //Measure duration between two beats
        lastBeat = millis();
    
        beatsPerMinute = 60 / (delta / 1000.0);           //Calculating the BPM
    
        if (beatsPerMinute < 255 && beatsPerMinute > 20)               //To calculate the average we strore some values (4) then do some math to calculate the average
        {
          rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
          rateSpot %= RATE_SIZE; //Wrap variable
    
          //Take average of readings
          beatAvg = 0;
          for (byte x = 0 ; x < RATE_SIZE ; x++)
            beatAvg += rates[x];
          beatAvg /= RATE_SIZE;
        }
      }
    
    }
    
      if (irValue < 7000){       //If no finger is detected it inform the user and put the average BPM to 0 or it will be stored for the next measure
        beatAvg=0;
        if(isCalling == 0){
            display.clearDisplay();
            display.setTextSize(0.5);                    
            display.setTextColor(WHITE);     
            display.setCursor(0,0);             
            display.println(patient_display);
            
            display.setCursor(90,0);                
            display.println("Active");         
            display.setCursor(30,10);                
            display.println("Please Place"); 
            display.setCursor(30,20);
            display.println(" your finger ");  
            display.display();
        }     
    }
}


void display_inavtive_mode(){
        display.clearDisplay();
        display.setTextSize(2);                    
        display.setTextColor(WHITE);             
        display.setCursor(13,0);                
        display.println("Inactive!"); 
        display.setTextSize(1);
        display.setCursor(0,20);
        display.println("Please contact staff");
        display.display();
        display.startscrollleft(0x0a, 0x0F);
        delay(6000); 
}


void iscalling_buzzer(){
  if(isCalling == 0){
    buzzer_init_time = millis();
  }
  if(isCalling == 1){
//    Serial.print("init_time: ");
//    Serial.println(buzzer_init_time);
    int f = millis();
    int gap = (f-buzzer_init_time)% buzzer_time_space;
    int gap1 = gap/5980;
//    Serial.print("iscalling: ");
//    Serial.print(isCalling);
//    Serial.print(" gap: ");
//    Serial.print(gap);
//    Serial.print(" gap1: ");
//    Serial.println(gap1);
    if(gap1 == 1){
      buzzer_st = f;
//      Serial.print("gap_time: ");
//      Serial.println(f);
      ledcWriteTone(0,200);
      delay(1000);
      ledcWriteTone(0,0);
    }else if(gap1 != 1 and millis()-buzzer_st == 1000){
//      Serial.println(millis());
      ledcWriteTone(0,0);
   }
  }
}


void display_iscalling(){
    display.clearDisplay();
    display.setTextSize(2);                    
    display.setTextColor(WHITE);             
    display.setCursor(13,0);                
    display.println("Calling!"); 
    display.setTextSize(1);
    display.setCursor(0,20);
    display.println("Please go to office");
    display.display();
}

void loop() {
  http_get_request();     
  iscalling_buzzer();     
  if(active_mode == 0){
       display_inavtive_mode();
  }
  else if(active_mode == 1){
       
       display.clearDisplay();  
       display.stopscroll();//Clear the display
       get_bpm_and_show();
       if(isCalling == 1){
          display_iscalling();
       }
       
  }
  
}
