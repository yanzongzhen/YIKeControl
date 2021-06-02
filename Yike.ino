#include <ESP8266WiFi.h>
#include <Ticker.h>

#ifndef STASSID
#define STASSID "一刻自习室"
#define STAPSK  "@666888."
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

Ticker flipper;
WiFiClient client;

int Relay = 2;
int Switch = 3;
bool connect_wifi = false;
bool maunal = false;
int lastSwicthState = LOW;

// lock cmd
String close_lock = "0107";
String open_lock = "0108";
String query_lock = "0109";

// lock state
String OPENED_STATE = "010A\r\n";
String CLOSED_STATE = "010B\r\n";
String CLOSE_RESP = "010C\r\n";
String OPEN_RESP = "010D\r\n";
String ping = "0100\r\n";
String CURRENT_STATE = "";

// tcp server
const char* host = "47.105.74.173";
const int port = 53409;

void ICACHE_RAM_ATTR  localControl();

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  pinMode(Relay, OUTPUT);
  digitalWrite(Relay, HIGH);
  pinMode(Switch, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Switch), localControl, CHANGE);
  delay(100);
  ConnnectWifi();
  flipper.attach(8, keepalive);
}

void loop()
{
  int val = digitalRead(Switch);
  if (val == LOW) {
    CURRENT_STATE = CLOSED_STATE;
  } else {
    CURRENT_STATE = OPENED_STATE;
  }
  lastSwicthState = val;
  if (connect_wifi)
  {
    while (!client.connected())
    {
      if (!client.connect(host, port))
      {
        Serial.println("wait connection.to server ......");
        int wstatus = WiFi.status();
        if (wstatus != WL_CONNECTED) {
          connect_wifi = false;
          break;
        }
      } else {
        char serialNo[10];
        uint32_t id = getChipId();
        sprintf(serialNo, "%ld", id);
        client.setTimeout(200);
        client.print(serialNo);
        sendMsg(CURRENT_STATE);
      }
    }

    while (client.connected())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\r\n');
        Serial.print("got cmd : ");
        Serial.println(line);
        if (line.equalsIgnoreCase(close_lock))
        {
          maunal = true;
          Serial.println("CLOSED .... ");
          sendMsg(closeLock());
        } else if (line.equalsIgnoreCase(open_lock))
        {
          maunal = true;
          Serial.println("OPEN .... ");
          sendMsg(openLock());
        } else if (line.equalsIgnoreCase(query_lock))
        {
          Serial.println("QUERY .... ");
          sendMsg(getRealTimeState());
        } else {
          break;
        }
      }
    }
  } else {
    ConnnectWifi();  
  }
}

String openLock()
{
  digitalWrite(Relay, LOW);
  delay(50);
  digitalWrite(Relay, HIGH);
  delay(50);
  CURRENT_STATE = OPENED_STATE;
  maunal = false;
  return OPEN_RESP;
}

// close lock and tiggle lockstate
String closeLock()
{
  digitalWrite(Relay, LOW);
  delay(50);
  digitalWrite(Relay, HIGH);
  delay(50);
  CURRENT_STATE = CLOSED_STATE;
  maunal = false;
  return CLOSE_RESP;
}

// get lock state
String getRealTimeState()
{
  return CURRENT_STATE;
}

// getChipId
uint32_t getChipId ()
{
  uint32_t coreChipId = ESP.getChipId();
  return coreChipId;
}

void keepalive()
{
  sendMsg(ping);
}

void localControl()
{
  int reading = digitalRead(Switch);
  int buttonState = reading;
  if (lastSwicthState == buttonState) {
    return;  
  }
  lastSwicthState = buttonState;
  if (buttonState == HIGH) {
    CURRENT_STATE = OPENED_STATE;
    if (maunal) {
      return;
    }
    sendMsg(CURRENT_STATE);
  } else if (buttonState == LOW) {
    CURRENT_STATE = CLOSED_STATE;
    if (maunal) {
      return;
    }
    sendMsg(CURRENT_STATE);
  }
}

void sendMsg(String msg)
{
  int str_len = 5;
  char msgbuffer[str_len];
  msg.toCharArray(msgbuffer, str_len);
  if (connect_wifi && client.connected())
  {
    Serial.print("send msg >> ");
    Serial.print(msgbuffer);
    Serial.print(" size: ");
    Serial.println(strlen(msgbuffer));
    client.setTimeout(200);
    client.print(msgbuffer);
    client.flush();
  }
}


void SmartConfig()
{
  WiFi.mode(WIFI_STA);
  Serial.println("\r\nWait for Smartconfig...");
  WiFi.beginSmartConfig();
  while (1)
  {
    Serial.print(".");
    delay(500);                   // wait for a second
    if (WiFi.smartConfigDone())
    {
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      break;
    }
  }
}


bool AutoConfig()
{
  WiFi.begin(ssid, password);
  for (int i = 0; i < 20; i++)
  {
    int wstatus = WiFi.status();
    if (wstatus == WL_CONNECTED)
    {
      Serial.println("WIFI SmartConfig Success");
      Serial.printf("SSID:%s", WiFi.SSID().c_str());
      Serial.printf(", PSW:%s\r\n", WiFi.psk().c_str());
      Serial.print("LocalIP:");
      Serial.print(WiFi.localIP());
      Serial.print(" ,GateIP:");
      Serial.println(WiFi.gatewayIP());
      return true;
    }
    else
    {
      Serial.print("current connect server status : ");
      Serial.println(wstatus);
      delay(1000);
    }
  }
  Serial.println("WIFI AutoConfig Faild!" );
  return false;
}


void ConnnectWifi() {
  if (!AutoConfig())
  {
    SmartConfig();
  }
  connect_wifi = true;
}
