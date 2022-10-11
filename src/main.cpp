#include <Arduino.h>
#include "DHT.h"
#include <U8g2lib.h>
 
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);    //Software I2C

#define DHTPIN 19 
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const int redPin = 5;
const int greenPin = 4;
const int bluePin = 2;
const int buttonPin = 15;
const int buzzPin = 16;
const int photoPin = 26; 
const int bumpPin = 17;
const int dirtPin = 25;

TaskHandle_t doorBell_t = NULL;
TaskHandle_t diningLight_t = NULL;
TaskHandle_t dht11_t = NULL;
TaskHandle_t bump_t = NULL;

void doorBellTones(void);
void createDoorBellTask();
void Task_doorBell(void *param);
void diningLight(void);
void createDiningLightTask();
void Task_diningLight(void *param);
void pinModeSetup();
void createAllTasks();
void lcdDhtInit();
void lcdDhtTask();
void createLcdDhtTask();
void Task_lcdDht(void *param);
void bumpTask();
void createBumpTask();
void Task_bump(void *param);

void setup() 
{ 
    Serial.begin(115200);
    pinModeSetup();
    lcdDhtInit();
    createAllTasks();
}

void loop() 
{

}

void pinModeSetup()
{
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    pinMode(buttonPin, INPUT);
    pinMode(buzzPin, OUTPUT);
    pinMode(photoPin, INPUT);
    pinMode(dirtPin, INPUT);
    pinMode(bumpPin, OUTPUT);
}

void createAllTasks()
{
    createDoorBellTask();
    createDiningLightTask();
    createLcdDhtTask();
    createBumpTask();
}

void doorBellTones(void)
{
    for(int i = 0; i < 100; i++)
    {
        digitalWrite(buzzPin, HIGH);
        vTaskDelay(1 / portTICK_PERIOD_MS);
        digitalWrite(buzzPin, LOW);
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    delay(10);
    for(int i = 0; i < 100; i++)
    {
        digitalWrite(buzzPin, HIGH);
        vTaskDelay(2 / portTICK_PERIOD_MS);
        digitalWrite(buzzPin, LOW);
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
}

void createDoorBellTask()
{
    xTaskCreate(
        Task_doorBell,
        "Task for Door Bell",
        8*1024,
        NULL,
        1,
        &doorBell_t
    );
}

void Task_doorBell(void *param) 
{
    while(1)
    {
        if(digitalRead(buttonPin) == HIGH)
        {
            doorBellTones();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        else
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        } 
    }
    vTaskDelete(NULL);
}

void diningLight(void)
{
    while(1)
    {
        if(analogRead(photoPin) > 2048)
        {
            for(int i = 0; i < 100; i++)
            {
                analogWrite(redPin, i);
                analogWrite(greenPin, i);
                analogWrite(bluePin, i);
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            for(int i = 100; i > 0; i--)
            {
                analogWrite(redPin, i);
                analogWrite(greenPin, i);
                analogWrite(bluePin, i);
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
        else
        {
            analogWrite(redPin, 0);
            analogWrite(greenPin, 0);
            analogWrite(bluePin, 0);
            delay(100);
        }
    }  
}

void createDiningLightTask()
{
    xTaskCreate(
        Task_diningLight,
        "Task for Dining Light",
        8*1024,
        NULL,
        2,
        &diningLight_t
    );
}

void Task_diningLight(void *param)
{
    while(1)
    {
        diningLight();
        delay(100);
    }
    vTaskDelete(NULL);
}

void lcdDhtInit()
{
    dht.begin();
    u8g2.begin();
}

void lcdDhtTask()
{
    while(1)
    {
        float h = dht.readHumidity();
        // Read temperature as Celsius (the default)
        float t = dht.readTemperature();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        float f = dht.readTemperature(close);

        // Check if any reads failed and exit early (to try again).
        if (isnan(h) || isnan(t) || isnan(f)) {
            Serial.println(F("Failed to read from DHT sensor!"));
            return;
        }

        // Compute heat index in Fahrenheit (the default)
        float hif = dht.computeHeatIndex(f, h);
        // Compute heat index in Celsius (isFahreheit = false)
        float hic = dht.computeHeatIndex(t, h, false);

        Serial.print(F("Humidity: "));
        Serial.print(h);
        Serial.print(F("%  Temperature: "));
        Serial.print(t);
        Serial.print(F("°C "));
        Serial.println();

        u8g2.clearBuffer(); 
        u8g2.setFont(u8g2_font_luBIS08_tf); 
        String humi = "Humi: " + String(h) + "%";
        String temp = "Temp: " + String(t) + "C";
        u8g2.drawStr(0, 10, humi.c_str());
        u8g2.drawStr(0, 30, temp.c_str());
        u8g2.sendBuffer();
        delay(100);
    }
}

void createLcdDhtTask()
{
    xTaskCreate(
        Task_lcdDht,
        "Task for LCD DHT",
        8*1024,
        NULL,
        3,
        &dht11_t
    );
}

void Task_lcdDht(void *param)
{
    while(1)
    {
        lcdDhtTask();
        delay(1000);
    }
    vTaskDelete(NULL);
}

void createBumpTask()
{
    xTaskCreate(
        Task_bump,
        "Task for Bump",
        8*1024,
        NULL,
        4,
        &bump_t
    );
}

void Task_bump(void *param)
{
    while(1)
    {
        Serial.println(analogRead(dirtPin));
        if(analogRead(dirtPin) > 2047)
        {
            digitalWrite(bumpPin, HIGH);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        else
        {
            digitalWrite(bumpPin, LOW);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelete(NULL);
}
