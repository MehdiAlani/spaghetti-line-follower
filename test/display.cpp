#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



bool spaghetti_state = false;
bool spaghetti_calib = false;




#define PIN_BOOT 0


#define PIN_PWMR 40
#define PIN_RF 42
#define PIN_RB 2
#define PIN_PWML 39
#define PIN_LB 1
#define PIN_LF 41

#define PIN_RGB_LED 48

#define PIN_ENCODER_RIGHT_A 21
#define PIN_ENCODER_RIGHT_B 47
 

#define PIN_ENCODER_LEFT_A 46
#define PIN_ENCODER_LEFT_B 45



#define PIN_SDA 17
#define PIN_SCL 16

#define PIN_MUX_SIG 14

#define PIN_MUX_S0 19
#define PIN_MUX_S1 20
#define PIN_MUX_S2 3
#define PIN_MUX_S3 48


#define LED_PIN 18


#define THRESH 10
#define DELAY_MS 3
static CRGB leds[1];  
void rgb_led_handle(){

  leds[0] = CRGB(255, 140, 40);
  for(int i = THRESH; i <= 255; i++){
    FastLED.setBrightness(i);
    FastLED.show();
    delay(DELAY_MS); 
  }

  for(int i = 255; i >= THRESH; i--){
    FastLED.setBrightness(i);
    FastLED.show();
    delay(DELAY_MS); 
  }
}

int esp_rand(){
  return (esp_random() % 4096);
}

char display_buff[200];
Adafruit_SSD1306 display(128, 64, &Wire1, -1);


void setup(){
  Serial.begin(115200);
  Wire1.begin(PIN_SDA,PIN_SCL,400000);

  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);


  

}

void loop(){

  sprintf(display_buff," %4d %4d %4d %4d  %4d %4d %4d %4d ",esp_rand(),esp_rand(),esp_rand(),esp_rand(),esp_rand(),esp_rand(),esp_rand(),esp_rand());
  display.setCursor(0, 49);
  display.print(display_buff);
  display.display();

  delay(500);
  display.clearDisplay();
}