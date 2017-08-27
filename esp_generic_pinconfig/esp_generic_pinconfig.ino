//code write by Moz for YouTube changel logMaker360, 24-11-2016
//code belongs to this video: https://youtu.be/nAUUdbUkJEI

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//-#include < ESP8266WebServer.h >

// Connect to the WiFi
const char *ssid = "_____";               //!!!!!!!!!!!!!!!!!!!!!
const char *password = "_____";         //!!!!!!!!!!!!!!!!!!!!!
const char *mqtt_server = "_____"; //!!!!!!!!!!!!!!!!!!!!!

////-ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

String localip = "elektronip";
String elektronname = "Elektron";
String currtime = "elektrontime";
String data = "elektrondata";
String client_id = "esp";

//Current Sensor ACS712 Variable Set
#define C_SENSOR1 A0
#define relay 2

//int pin1 = pin1;

byte DPin[] = {D1, D2, D3, D4, D5, D6, D7, D8}; // list of digital/PWM pins
//const byte APin[] = {A0};                                // list of analog input pins
int DPinMode[] = {1, 1, 1, 1, 1, 1, 1, 1};  // input = 0, output = 1
int DPinState[] = {1, 1, 1, 1, 1, 1, 1, 1}; // 0 or 1
//int APinState[] = [0];                                   // 0 to 255 (1023)

char pinTopicBuffer[20];
String pinTopicBase = String("thing/esp/pin_d/");

const int DPinLength = 8;
const int JSONBUFFERSIZE = 350;
int ps = 0;

void setPinModes()
{
    // DIGITAL PIN MODE
    for (int i = 0; i < DPinLength; i++)
        pinMode(DPin[i], DPinMode[i]);

    Serial.println("todo modeado");

    // ANALOG NOT NEEDED FOR ESP8266 WEMOS, only has a input
}
void setPinValues()
{
    // DIGITAL PIN STATE
    for (int i = 0; i < DPinLength; i++)
        if (DPinMode[i] == 1) // If is mode OUTPUT
            digitalWrite(DPin[i], DPinState[i]);

    Serial.println("todo seteado");
    // ANALOG NOT NEEDED FOR ESP8266 WEMOS, only has a input
}
void readAllDigital()
{
    // DIGITAL PIN STATE
    for (int i = 0; i < DPinLength; i++)
        if (DPinMode[i] == 0) // If is mode INPUT
        {
            int value = digitalRead(DPin[i]);
            if (DPinState[i] != value)
            {
                DPinState[i] = value;

                char data[5];
                String payload = String(value);
                payload.toCharArray(data, (payload.length() + 1));

                String pinTopic = pinTopicBase + String(i); 
                pinTopic.toCharArray(pinTopicBuffer, (pinTopic.length() + 1));

                client.publish(pinTopicBuffer, data);
                Serial.print("published on ");
                Serial.println(pinTopic);
                Serial.print("value ");
                Serial.println(payload);
            }
        }
}

void setup()
{
    Serial.begin(115200);
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    setPinModes();
    setPinValues();
}

void loop()
{
    if (!client.connected())
        reconnect();

    IPAddress ip = WiFi.localIP();
    localip = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    readAllDigital();
    client.loop();
}

/***********************************************************/
/****************** Connect | Reconnect ********************/
/***********************************************************/
void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("esp"))
        {
            Serial.println("connected");
            // ... and subscribe to topic
            client.subscribe("thing/esp/config");
            client.subscribe("thing/esp/set_d");
            //client.subscribe("thing/esp/set_a");

            //-client.subscribe("thing/esp/get");
            //-client.subscribe("ledStatus");
            char data[80];
            IPAddress ip = WiFi.localIP();
            localip = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
            String payload = "{\"ip\":\"" + localip + "\",\"time\":\"" + currtime + "\",\"name\":\"" + elektronname + "\",\"data\":\"0\"}";
            payload.toCharArray(data, (payload.length() + 1));

            client.publish("esp8266status", data);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000); // TODO: change for a yield()
        }
    }
}

/***********************************************************/
/*********************** On Message ************************/
/***********************************************************/
void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    // SET CONFIG
    //const char* json = "{\"d_mode\":[0,0,0,0,0,0,0,0,0],\"d_state\":[0,0,0,0,0,0,0,0,0]}";
    if (strcmp(topic, "thing/esp/config") == 0)
    {
        Serial.println("RECIVED CONFI:");
        Serial.println((char *)payload);

        char new_payload[length + 1];
        for (int i = 0; i < length; i++)
        {
            new_payload[i] = (char)payload[i]; //Serial.print((char)payload[i]);
        }
        new_payload[length] = '\0';
        // Print values.
        Serial.println(String(new_payload));

        StaticJsonBuffer<JSONBUFFERSIZE> jsonBuffer; // MAX BUFFER, EXPECTED SIZE UP TO 200
                                                     //        String msgString = String((char*)payload);
        //new_payload.toCharArray(data, (payload.length() + 1));

        // Deserialize the JSON string
        JsonObject &root = jsonBuffer.parseObject(new_payload);
        JsonArray &d_mode = root["d_mode"];
        JsonArray &d_status = root["d_status"];

        // Test if parsing succeeds.
        if (!root.success())
        {
            Serial.println("parseObject() failed");
            return;
        }
        else
        {
            // Most of the time, you can rely on the implicit casts.
            // In other case, you can do root["time"].as<long>();
            //const char* sensor = root["sensd_modeor"];
            //long time = root["time"];
            for (int i = 0; i < DPinLength; i++)
                DPinMode[i] = d_mode[i];
            for (int i = 0; i < DPinLength; i++)
                DPinState[i] = d_status[i];

            setPinModes();
            setPinValues();
        }
    }

    // SET VALUE IF PIN
    //const char* json = "{\"d_state\":[0,0,0,0,0,0,0,0,0]}";
    if (strcmp(topic, "thing/esp/set_d") == 0)
    {
        Serial.println("RECIVED SET:");
        Serial.println((char *)payload);

        char new_payload[length + 1];
        for (int i = 0; i < length; i++)
        {
            new_payload[i] = (char)payload[i]; //Serial.print((char)payload[i]);
        }
        new_payload[length] = '\0';
        // Print values.
        Serial.println(String(new_payload));

        StaticJsonBuffer<JSONBUFFERSIZE> jsonBuffer; // MAX BUFFER, EXPECTED SIZE UP TO 200
                                                     //        String msgString = String((char*)payload);
        //new_payload.toCharArray(data, (payload.length() + 1));

        // Deserialize the JSON string
        JsonObject &root = jsonBuffer.parseObject(new_payload);
        int pin = root["pin"];
        int value = root["value"];

        // Test if parsing succeeds.
        if (!root.success())
        {
            Serial.println("parseObject() failed");
            return;
        }
        else
        {
            // Most of the time, you can rely on the implicit casts.
            // In other case, you can do root["time"].as<long>();
            DPinState[pin] = value;
            digitalWrite(DPin[pin], value);
        }
    }

    // GET MODES AND VALUES
    if (strcmp(topic, "thing/get") == 0)
    {
    }
    Serial.println();
}

//void changeValue(int pin, )

