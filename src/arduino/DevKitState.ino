#include "Arduino.h"
#include "AZ3166WiFi.h"
#include "DevKitMQTTClient.h"
#include "SystemVersion.h"
#include "Sensor.h"
#include "parson.h"

DevI2C *ext_i2c;
LSM6DSLSensor *acc_gyro;
HTS221Sensor *ht_sensor;
LIS2MDLSensor *magnetometer;
IRDASensor *IrdaSensor;
LPS22HBSensor *pressureSensor;
RGB_LED rgbLed;

static bool hasWifi = false;
static int userLEDState = 0;
static int rgbLEDState = 0;
static int rgbLEDR = 0;
static int rgbLEDG = 0;
static int rgbLEDB = 0;

static void InitWifi()
{
  Screen.print(2, "Connecting...");

  if (WiFi.begin() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    Screen.print(1, ip.get_address());
    hasWifi = true;
    Screen.print(2, "Running... \r\n");
  }
  else
  {
    hasWifi = false;
    Screen.print(1, "No Wi-Fi\r\n ");
  }
}
void parseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message)
{
    JSON_Value *root_value;
    root_value = json_parse_string(message);
    if (json_value_get_type(root_value) != JSONObject)
    {
        if (root_value != NULL)
        {
            json_value_free(root_value);
        }
        LogError("parse %s failed", message);
        return;
    }
    JSON_Object *root_object = json_value_get_object(root_value);

    if (updateState == DEVICE_TWIN_UPDATE_COMPLETE)
    {
        JSON_Object *desired_object = json_object_get_object(root_object, "desired");
        if (desired_object != NULL)
        {
          if (json_object_has_value(desired_object, "userLEDState"))
          {
            userLEDState = json_object_get_number(desired_object, "userLEDState");
          }
          if (json_object_has_value(desired_object, "rgbLEDState"))
          {
            rgbLEDState = json_object_get_number(desired_object, "rgbLEDState");
          }
          if (json_object_has_value(desired_object, "rgbLEDR"))
          {
            rgbLEDR = json_object_get_number(desired_object, "rgbLEDR");
          }
          if (json_object_has_value(desired_object, "rgbLEDG"))
          {
            rgbLEDG = json_object_get_number(desired_object, "rgbLEDG");
          }
          if (json_object_has_value(desired_object, "rgbLEDB"))
          {
            rgbLEDB = json_object_get_number(desired_object, "rgbLEDB");
          }
        }
    }
    else
    {
      if (json_object_has_value(root_object, "userLEDState"))
      {
        userLEDState = json_object_get_number(root_object, "userLEDState");
      }
      if (json_object_has_value(root_object, "rgbLEDState"))
      {
        rgbLEDState = json_object_get_number(root_object, "rgbLEDState");
      }
      if (json_object_has_value(root_object, "rgbLEDR"))
      {
        rgbLEDR = json_object_get_number(root_object, "rgbLEDR");
      }
      if (json_object_has_value(root_object, "rgbLEDG"))
      {
        rgbLEDG = json_object_get_number(root_object, "rgbLEDG");
      }
      if (json_object_has_value(root_object, "rgbLEDB"))
      {
        rgbLEDB = json_object_get_number(root_object, "rgbLEDB");
      }
    }

    if (rgbLEDState == 0)
    {
      rgbLed.turnOff();
    }
    else
    {
      rgbLed.setColor(rgbLEDR, rgbLEDG, rgbLEDB);
    }

    pinMode(LED_USER, OUTPUT);
    digitalWrite(LED_USER, userLEDState);
    json_value_free(root_value);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  parseTwinMessage(updateState, temp);
  free(temp);
}

void setup()
{
  rgbLed.turnOff();
  Screen.init();
  Screen.print(0, "IoT DevKit");
  Screen.print(2, "Initializing...");
  Screen.print(3, " > WiFi");
  hasWifi = false;
  InitWifi();
  if (!hasWifi)
  {
    return;
  }

  Screen.print(3, " > IoT Hub");
  DevKitMQTTClient_Init(true);
  DevKitMQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
}

bool i2cError = false;
int sensorMotion;
int sensorPressure;
int sensorMagnetometer;
int sensorHumidityAndTemperature;
int sensorIrda;

void loop()
{
  DevKitMQTTClient_Check();
  const char *firmwareVersion = getDevkitVersion();
  const char *wifiSSID = WiFi.SSID();
  int wifiRSSI = WiFi.RSSI();
  const char *wifiIP = (const char *)WiFi.localIP().get_address();
  const char *wifiMask = (const char *)WiFi.subnetMask().get_address();
  byte mac[6];
  char macAddress[18];
  WiFi.macAddress(mac);
  snprintf(macAddress, 18, "%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  try
  {
    ext_i2c = new DevI2C(D14, D15);
    i2cError = false;
  }
  catch(int error)
  {
    i2cError = true;
    sensorMotion = 0;
    sensorPressure = 0;
    sensorMagnetometer = 0;
    sensorHumidityAndTemperature = 0;
    sensorIrda = 0;
  }

  int sensorInitResult;

  if (!i2cError)
  {
    try
    {
      acc_gyro = new LSM6DSLSensor(*ext_i2c, D4, D5);
      sensorInitResult = acc_gyro->init(NULL);
      acc_gyro->enableAccelerator();
      acc_gyro->enableGyroscope();

      if (sensorInitResult == 0)
      {
        sensorMotion = 1;
      }
      else
      {
        sensorMotion = 0;
      }
    }
    catch(int error)
    {
      sensorMotion = 0;
    }

    try
    {
      ht_sensor = new HTS221Sensor(*ext_i2c);
      sensorInitResult = ht_sensor->init(NULL);

      if (sensorInitResult == 0)
      {
        sensorHumidityAndTemperature = 1;
      }
      else
      {
        sensorHumidityAndTemperature = 0;
      }
    }
    catch(int error)
    {
      sensorHumidityAndTemperature = 0;
    }

    try
    {
      magnetometer = new LIS2MDLSensor(*ext_i2c);
      sensorInitResult = magnetometer->init(NULL);

      if (sensorInitResult == 0)
      {
        sensorMagnetometer = 1;
      }
      else
      {
        sensorMagnetometer = 0;
      }
    }
    catch(int error)
    {
      sensorMagnetometer = 0;
    }

    try
    {
      pressureSensor = new LPS22HBSensor(*ext_i2c);
      sensorInitResult = pressureSensor -> init(NULL);

      if (sensorInitResult == 0)
      {
        sensorPressure = 1;
      }
      else
      {
        sensorPressure = 0;
      }
    }
    catch(int error)
    {
      sensorPressure = 0;
    }

    try
    {
      IrdaSensor = new IRDASensor();
      sensorInitResult = IrdaSensor->init();

      if (sensorInitResult == 0)
      {
        sensorIrda = 1;
      }
      else
      {
        sensorIrda = 0;
      }
    }
    catch(int error)
    {
      sensorIrda = 0;
    }
  }

  if (rgbLEDState == 0)
  {
    rgbLed.turnOff();
  }
  else
  {
    rgbLed.setColor(rgbLEDR, rgbLEDG, rgbLEDB);
  }

  pinMode(LED_USER, OUTPUT);
  digitalWrite(LED_USER, userLEDState);

  char state[500];
  snprintf(state, 500, "{\"firmwareVersion\":\"%s\",\"wifiSSID\":\"%s\",\"wifiRSSI\":%d,\"wifiIP\":\"%s\",\"wifiMask\":\"%s\",\"macAddress\":\"%s\",\"sensorMotion\":%d,\"sensorPressure\":%d,\"sensorMagnetometer\":%d,\"sensorHumidityAndTemperature\":%d,\"sensorIrda\":%d}", firmwareVersion, wifiSSID, wifiRSSI, wifiIP, wifiMask, macAddress, sensorMotion, sensorPressure, sensorMagnetometer, sensorHumidityAndTemperature, sensorIrda);
  DevKitMQTTClient_ReportState(state);
  delay(5000);
}
