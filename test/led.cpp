#include <Arduino.h>
#include <FastLED.h>


static CRGB leds[1];  
void rgb_led_init(){
  FastLED.addLeds<WS2812, 48, GRB>(leds, 1);
}

uint8_t red = 0,green = 0,blue = 0;
void rgb_led_out(){
  leds[0].r = red;
  leds[0].g = green;
  leds[0].b = blue;
  FastLED.show();
} 

void setup() {
    Serial.begin(115200);
    rgb_led_init();
    
    pinMode(48,OUTPUT);
    digitalWrite(48,LOW);
    digitalWrite(48,HIGH);
    digitalWrite(48,LOW);
    digitalWrite(48,HIGH);

    rgb_led_init();
    red = 255;
    green = 0;
    blue = 0;
    rgb_led_out();
}

void loop() {


  
}





