#include <Arduino.h>



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



#define PIN_SDA 4
#define PIN_SCL 5

#define PIN_MUX_SIG 14

#define PIN_MUX_S0 19
#define PIN_MUX_S1 20
#define PIN_MUX_S2 3
#define PIN_MUX_S3 48

void pinout_init(){
    pinMode(PIN_RF,OUTPUT);
    pinMode(PIN_RB,OUTPUT);
    pinMode(PIN_LF,OUTPUT);
    pinMode(PIN_LB,OUTPUT);


    pinMode(PIN_BOOT,INPUT_PULLUP);


    pinMode(PIN_MUX_SIG,INPUT);
    pinMode(PIN_MUX_S0,OUTPUT);
    pinMode(PIN_MUX_S1,OUTPUT);
    pinMode(PIN_MUX_S2,OUTPUT);
    pinMode(PIN_MUX_S3,OUTPUT);
    digitalWrite(PIN_MUX_S0,LOW);
    digitalWrite(PIN_MUX_S1,LOW);
    digitalWrite(PIN_MUX_S2,LOW);
    digitalWrite(PIN_MUX_S3,LOW);


    pinMode(PIN_ENCODER_RIGHT_A,INPUT);
    pinMode(PIN_ENCODER_RIGHT_B,INPUT);
    pinMode(PIN_ENCODER_LEFT_A,INPUT);
    pinMode(PIN_ENCODER_LEFT_B,INPUT);
}




void mux_select(uint8_t pin){
    digitalWrite(PIN_MUX_S0,pin & 0x01);
    digitalWrite(PIN_MUX_S1,pin & 0x02);
    digitalWrite(PIN_MUX_S2,pin & 0x04);
    digitalWrite(PIN_MUX_S3,pin & 0x08);
}


int floor(int nb,int min,int max){
    if(nb <= min) return min;
    else if(nb >= max) return max;
    return nb;
}



#define DEBOUNCE_MS 300

bool boot_button = false;
uint32_t boot_debounce_ms = 0;

void boot_button_update(){
    bool state = digitalRead(PIN_BOOT); 
    if(state == 0){
        if((millis() - boot_debounce_ms > DEBOUNCE_MS) && boot_button){
            spaghetti_state = !spaghetti_state; 
            boot_debounce_ms = millis();
        }
        boot_button  = state;
    }
    else{
        boot_button =  state;
    }      
}

bool mux_button_read(uint8_t channel){
    mux_select(channel);
    pinMode(PIN_MUX_SIG,INPUT_PULLUP);
    bool state = digitalRead(PIN_MUX_SIG);
    pinMode(PIN_MUX_SIG,INPUT);
    return state;
}

bool start_button = false;
uint32_t start_debounce_ms = 0;

void start_button_update(){
    bool state = mux_button_read(14); 
    if(state == 0){
        if((millis() - start_debounce_ms > DEBOUNCE_MS) && start_button){
            spaghetti_state = !spaghetti_state; 
            start_debounce_ms = millis();
        }
        start_button  = state;
    }
    else{
        start_button =  state;
    }      
}

bool calib_button = false;
uint32_t calib_debounce_ms = 0;

void calib_button_update(){
    bool state = mux_button_read(15); 
    if(state == 0){
        if((millis() - calib_debounce_ms > DEBOUNCE_MS) && calib_button){
            spaghetti_calib = !spaghetti_calib; 
            calib_debounce_ms = millis();
        }
        calib_button  = state;
    }
    else{
        calib_button =  state;
    }      
}


#define RES_1 10026.0
#define RES_2 3238.0
float batt_voltage = 0;
void batt_voltage_update(){
  mux_select(13);
  int raw = analogRead(PIN_MUX_SIG);
  batt_voltage = (1 + RES_1 / RES_2) * ((raw / 4095.0) * 3.3);
}




  //==================================================================================================Motor Part==========================================================================================================================

  #define MOTOR_RIGHT_CHANNEL 0
  #define MOTOR_LEFT_CHANNEL  1



  #define MOTOR_RES 10
  #define MOTOR_FREQ_KHZ 5
  #define MAX_DUTY 1023


  int duty_right = 0;
  int duty_left = 0;

void motors_init(){


    digitalWrite(PIN_RF,HIGH);
    digitalWrite(PIN_RB,LOW);
    digitalWrite(PIN_LF,HIGH);
    digitalWrite(PIN_LB,LOW);

    ledcSetup(MOTOR_RIGHT_CHANNEL,MOTOR_FREQ_KHZ * 1000,MOTOR_RES);
    ledcAttachPin(PIN_PWMR,MOTOR_RIGHT_CHANNEL);
    ledcWrite(MOTOR_RIGHT_CHANNEL,0);


    ledcSetup(MOTOR_LEFT_CHANNEL,MOTOR_FREQ_KHZ * 1000,MOTOR_RES);
    ledcAttachPin(PIN_PWML,MOTOR_LEFT_CHANNEL);
    ledcWrite(MOTOR_LEFT_CHANNEL,0);


}

void motors_update(){


  if(duty_right >= 0){
    digitalWrite(PIN_RF,HIGH);
    digitalWrite(PIN_RB,LOW);
  }
  else{
    digitalWrite(PIN_RF,LOW);
    digitalWrite(PIN_RB,HIGH);
  }



  if(duty_left >= 0){
    digitalWrite(PIN_LF,HIGH);
    digitalWrite(PIN_LB,LOW);
  }
  else{
    digitalWrite(PIN_LF,LOW);
    digitalWrite(PIN_LB,HIGH);
  }

  ledcWrite(MOTOR_RIGHT_CHANNEL,floor(abs(duty_right),0,MAX_DUTY));
  ledcWrite(MOTOR_LEFT_CHANNEL  ,floor(abs(duty_left),0,MAX_DUTY));
}


//=============================================================================================Odemtry Part=============================================================================================================

#define MIN_TIME_US 4000

int32_t right_ticks = 0;
uint32_t right_tmr_us = 0;
uint32_t right_diff_us = 0;
bool right_dir = 0;


void ENCODER_ISR_RIGHT(){
  if(digitalRead(PIN_ENCODER_RIGHT_B) == 0){
    right_ticks++;
    right_dir = 0;
  }
  else{
    right_ticks--;
    right_dir = 1;
  }
}



int32_t left_ticks = 0;
uint32_t left_tmr_us = 0;
uint32_t left_diff_us = 0;

bool left_dir = 0;


void ENCODER_ISR_LEFT(){
  if(digitalRead(PIN_ENCODER_LEFT_B) == 0){
    left_ticks++;
    left_dir = 0;
  }
  else{
    left_ticks--;
    left_dir = 1;
  }
  
}



// Distance in MM
#define WHEEL_RADUIS_CM 3.2
#define ENCODER_STEPS_PER_REV 90.0

//in CM

float right_distance = 0;
float left_distance= 0;
float distance = 0;



void odemtry_init(){
  right_ticks = 0;  
  right_distance = 0;
  left_ticks = 0;
  left_distance = 0;
  distance = 0;



   attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_RIGHT_A), ENCODER_ISR_RIGHT, RISING);
   attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_LEFT_A), ENCODER_ISR_LEFT, RISING);
}



void odemtry_handle(){
    right_distance = (WHEEL_RADUIS_CM * 2 * PI) * (right_ticks / ENCODER_STEPS_PER_REV);
    left_distance = (WHEEL_RADUIS_CM * 2 * PI) * (left_ticks / ENCODER_STEPS_PER_REV);
    distance = (right_distance + left_distance) / 2;

}


void setup() {
  Serial.begin(115200);
  pinout_init();
  motors_init();
  odemtry_init();
}

uint32_t delay_ms = 0;
#define MOTOR_DELAY_MS 500
int nb;
void loop() {
  if(Serial.available()){
    nb = Serial.readStringUntil('\n').toInt();
  }

  if(spaghetti_state){
    duty_left = nb;
    duty_right = nb;
    motors_update();
  }
  else{
    duty_left = 0;
    duty_right = 0;
    motors_update();
  }

  if(millis() - delay_ms > MOTOR_DELAY_MS){
    Serial.println("spaghetti_state : " + String(spaghetti_state) + " spaghetti_calib: " + String(spaghetti_calib));
    Serial.println("right_ticks: " + String(right_ticks) + " left_ticks: " + String(left_ticks));
    Serial.println(batt_voltage);
    delay_ms = millis();
  }


  
  batt_voltage_update();
  boot_button_update();
  calib_button_update();
  start_button_update();
}































