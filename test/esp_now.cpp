#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>


void onRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  Serial.println("Revcieved");
}


void setup(){
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);

    if(esp_now_init() != ESP_OK){
        Serial.println("esp_now Failed");
        ESP.restart();
    }

    esp_now_register_recv_cb(onRecv);



}


void loop(){

}