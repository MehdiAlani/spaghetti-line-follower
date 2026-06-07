#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>
#include <esp_now.h>
#include <WiFi.h>

bool spaghetti_state = false;
bool spaghetti_calib = false;
int section_id = 0;
uint32_t start_time = 0, finish_time = 0;

//================================================================================================ESP_NOW Controller

// ESP-NOW on core 0

void onRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  spaghetti_state = !spaghetti_state;  
}




//================================================================================================Pinout
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



#define PIN_SDA_0 4
#define PIN_SCL_0 5

#define PIN_SDA_1 17
#define PIN_SCL_1 16

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

float angle_floor(float angle){
  if(angle > 360) return 0;
  if(angle < 0) return 360;
  return angle;
}



#define DEBOUNCE_MS 300

bool boot_button = false;
uint32_t boot_debounce_ms = 0;

void boot_button_update(){
    bool state = digitalRead(PIN_BOOT); 
    if(state == 0){
        if((millis() - boot_debounce_ms > DEBOUNCE_MS) && boot_button){
            // spaghetti_state = !spaghetti_state; 
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
            if(!spaghetti_state) spaghetti_calib = !spaghetti_calib;  // This To ovide spaghetti headacke 
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

//======================================================================================================IMU Part=============================================================================================

//========================================REGISTERS================================================
#define GYRO_CONFIG 0x1B
#define ACCE_CONFIG 0x1C
#define ACCE_X 0x3B 
#define ACCE_Y 0x3D
#define GYRO_Z 0x47

#define MPU_ADDR 0x68 
#define PWR_MGMT_1 0x6B

//======================================Constants Vector============================================
#define GYRO_250 0x00
#define GYRO_500 0x08
#define GYRO_1000 0x10
#define GYRO_2000 0x18

#define ACCE_2G 0x00
#define ACCE_4G 0x08
#define ACCE_8G 0x10
#define ACCE_16G 0x18


#define GYRO_250_COFF 131.0
#define GYRO_500_COFF 65.5
#define GYRO_1000_COFF 32.8
#define GYRO_2000_COFF 16.4

#define ACCE_2G_COFF 16384.0
#define ACCE_4G_COFF 8192.0
#define ACCE_8G_COFF 4096.0
#define ACCE_16G_COFF 2048.0


// IMU Global 

float imu_speed_z = 0;
float imu_z = 0;
float imu_z_raw  = 0;
int16_t gyro_z,calibrate_bias;
uint32_t mpu_time_us;


void imu_init(){

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(PWR_MGMT_1);          
  Wire.write(0);
  Wire.endTransmission(true);


  Wire.beginTransmission(MPU_ADDR);
  Wire.write(GYRO_CONFIG);          
  Wire.write(GYRO_1000);           
  Wire.endTransmission(true);

}



#define IMU_SAMPLE_TIME_US 1000
void imu_read(){

    if(esp_timer_get_time() - mpu_time_us > IMU_SAMPLE_TIME_US){
        int16_t diff;
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(GYRO_Z);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, 2, true);
        gyro_z = Wire.read() << 8 | Wire.read();
        Wire.endTransmission(true);

        diff = esp_timer_get_time() - mpu_time_us;
        mpu_time_us = esp_timer_get_time();

        imu_speed_z = (gyro_z - calibrate_bias) / GYRO_1000_COFF;
        
        imu_z_raw += imu_speed_z * (diff) / 1000000.0;

        imu_z  += imu_speed_z * (diff) / 1000000.0;

        imu_z = angle_floor(imu_z);
    }
}

#define NB_SAMPLES 2000
void imu_calibrate(){
  int32_t sum = 0;
  calibrate_bias = 0;
  for(int i = 0; i < NB_SAMPLES; i++){
    imu_read();
    sum += gyro_z;
    delayMicroseconds(IMU_SAMPLE_TIME_US);
  }
  calibrate_bias = sum / NB_SAMPLES;
  imu_z_raw = 0;
  imu_z = 0;
}


//=============================================================================================Odometry Part=============================================================================================================

#define MIN_TIME_US 4000

int32_t right_ticks = 0;
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



void odometry_init(){
  right_ticks = 0;  
  right_distance = 0;
  left_ticks = 0;
  left_distance = 0;
  distance = 0;


  imu_z = 0;
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_RIGHT_A), ENCODER_ISR_RIGHT, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_LEFT_A), ENCODER_ISR_LEFT, RISING);
}



void odometry_update(){
    right_distance = (WHEEL_RADUIS_CM * 2 * PI) * (right_ticks / ENCODER_STEPS_PER_REV);
    left_distance = (WHEEL_RADUIS_CM * 2 * PI) * (left_ticks / ENCODER_STEPS_PER_REV);
    distance = (right_distance + left_distance) / 2;

}



//================================================================================================IR Part=============================================================================================




#define IR_NB 9
int ir_array_black[IR_NB];
int ir_array_white[IR_NB]; 
int ir_array_raw[IR_NB]; 
float ir_array[IR_NB]; 


 
void ir_read(){
    for(uint8_t i = 0; i < IR_NB; i++){
        mux_select(i);
        ir_array_raw[i] = analogRead(PIN_MUX_SIG);
        ir_array[i] = ((ir_array_raw[i] - ir_array_black[i]) * ((4095.0) / (ir_array_white[i] - ir_array_black[i])));
    }
}


//===================================================================================File System Part========================================================================================

void fs_init(){
  LittleFS.begin();
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
      Serial.println("File Name: " + String(file.name()) + "\t Size: " + String(file.size()));
      file = root.openNextFile();
  }
}

void fs_ir_claib_save(){
  File file = LittleFS.open("/calib.txt","w");
  for(uint8_t i = 0; i < IR_NB; i++){
    file.print(String(ir_array_white[i]) + " ");
  }

  file.println();
  for(uint8_t i = 0; i < IR_NB; i++){
    file.print(String(ir_array_black[i]) + " ");
  }
  file.println();

  file.close();
}




void fs_ir_claib_load(){
  File file = LittleFS.open("/calib.txt","r");
  String content = file.readString();
  String line = content.substring(0,content.indexOf('\n'));
  for (uint8_t i = 0; i < IR_NB;i++){
    ir_array_white[i] = line.substring(0,line.indexOf(" ")).toInt();
    line = line.substring(line.indexOf(" ") + 1);
  }
  content= content.substring(content.indexOf('\n') + 1);
  line = content.substring(0,content.indexOf('\n'));
  for (uint8_t i = 0; i < IR_NB;i++){
    ir_array_black[i] = line.substring(0,line.indexOf(" ")).toInt();
    line = line.substring(line.indexOf(" ") + 1);
  } 
}



//========================================================================================Controllers=====================================================================================



//=========================================IR CONTROLLER
#define PID_DELAY_US 1000

float IR_BASE_DUTY = 350;
float IR_KP = 0.5;
float IR_KD = 0;
float IR_KI = 0;

float ir_error;
float ir_p = 0,ir_i = 0,ir_d = 0; 
float ir_pid = 0;
float ir_prev_error = 0;

uint32_t pid_us = 0;
void ir_controller(){
    if(esp_timer_get_time() - pid_us > PID_DELAY_US){
        float diff_time_s = (esp_timer_get_time() - pid_us) / 1000000.0;
        ir_p = IR_KP * ir_error;
        ir_i += IR_KI * (ir_error * (diff_time_s));
        ir_d = IR_KD * (ir_error - ir_prev_error) / (diff_time_s) ;

        if(ir_i > MAX_DUTY) ir_i = MAX_DUTY;
        else if(ir_i < -MAX_DUTY) ir_i =-MAX_DUTY;

        ir_pid = ir_p + ir_i + ir_d;
        
        
        ir_prev_error = ir_error;
        duty_right = IR_BASE_DUTY - ir_pid;
        duty_left = IR_BASE_DUTY  + ir_pid;

        pid_us = esp_timer_get_time();
    }
}




//=========================================IR DETECT 
float DISTANCE_THRESH = 4;


int BLACK_THRESH = 1000;

bool detect_right_line_black = false;
bool detect_left_line_black = false;

bool verify_right_line_black = false;
bool verify_left_line_black = false;


bool precence_right_line_black = false;
bool precence_left_line_black = false;


int count_right_line_black = 0;
int count_left_line_black = 0;

float distance_right_thresh_black = 0;
float distance_left_thresh_black = 0;



int WHITE_THRESH = 2000;

bool detect_right_line_white = false;
bool detect_left_line_white = false;

bool verify_right_line_white = false;
bool verify_left_line_white = false;


bool precence_right_line_white = false;
bool precence_left_line_white = false;


int count_right_line_white = 0;
int count_left_line_white = 0;

float distance_right_thresh_white = 0;
float distance_left_thresh_white = 0;





#define THRESH_TIME 10 

void ir_detect(){
    // Black Part 
    

    if(detect_right_line_black){
        if(!verify_right_line_black){
            distance_right_thresh_black = distance;
            verify_right_line_black = true;
        }
        if(abs(distance - distance_right_thresh_black) > DISTANCE_THRESH){
            precence_right_line_black = true;
        }
    }
    else{
        verify_right_line_black = false;
        if(precence_right_line_black){
            count_right_line_black++;
            precence_right_line_black = false;
        }
    }


    if(detect_left_line_black){
        if(!verify_left_line_black){
            distance_left_thresh_black = distance;
            verify_left_line_black = true;
        }
        if(abs(distance - distance_left_thresh_black) > DISTANCE_THRESH){
            precence_left_line_black = true;
        }
    }
    else{
        verify_left_line_black = false;
        if(precence_left_line_black){
            count_left_line_black++;
            precence_left_line_black = false;
        }
    }


    // White Part 

    if(detect_right_line_white){
        if(!verify_right_line_white){
            distance_right_thresh_white = distance;
            verify_right_line_white = true;
        }
        if(abs(distance - distance_right_thresh_white) > DISTANCE_THRESH){
            precence_right_line_white = true;
        }
    }
    else{
        verify_right_line_white = false;
        if(precence_right_line_white){
            count_right_line_white++;
            precence_right_line_white = false;
        }
    }


    if(detect_left_line_white){
        if(!verify_left_line_white){
            distance_left_thresh_white = distance;
            verify_left_line_white = true;
        }
        if(abs(distance - distance_left_thresh_white) > DISTANCE_THRESH){
            precence_left_line_white = true;
        }
    }
    else{
        verify_left_line_white = false;
        if(precence_left_line_white){
            count_left_line_white++;
            precence_left_line_white = false;
        }
    }


}



//===============================================================IMU CONTROLLERS

float angle_diff(float angle1 , float angle2){
    float diff1 = abs(angle1 - angle2);
    float diff2 = 360 - diff1;
    if(diff2 <= diff1) return diff2;
    return diff1;
}

float IMU_FORWARD_DUTY = 300;
float IMU_FORWARD_KP = 100;
float IMU_FORWARD_KD = 0;
float IMU_FORWARD_KI = 0;
float imu_forward_p = 0; 

void imu_forward(float move_angle){
  imu_forward_p = IMU_FORWARD_KP * (imu_z_raw - move_angle);
  
  duty_left  =  IMU_FORWARD_DUTY + imu_forward_p;
  duty_right =  IMU_FORWARD_DUTY - imu_forward_p;

}




#define ANGLE_EPSELON 0
#define TURN_BASE_DUTY 700
void imu_turn(float angle){

    float start_angle = imu_z;

    if(angle > 0){
        duty_right = TURN_BASE_DUTY;
        duty_left = -TURN_BASE_DUTY;
    }
    else{
        duty_right = -TURN_BASE_DUTY;
        duty_left = TURN_BASE_DUTY;
    }
    motors_update();
    while(angle_diff(imu_z,start_angle) < abs(angle) - ANGLE_EPSELON){
        vTaskDelay(1);
    }
}






//===================================================================================Display & RGB LED Part======================================================================
Adafruit_SSD1306 display(128, 64, &Wire1, -1);
char display_buff[200];

void display_init(){
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
}


static CRGB leds[1];  
void rgb_led_init(){
  FastLED.addLeds<WS2812, PIN_RGB_LED, GRB>(leds, 1);
}

uint8_t red = 0,green = 0,blue = 0;
void rgb_led_out(){
  leds[0].r = red;
  leds[0].g = green;
  leds[0].b = blue;
  FastLED.show();
} 


uint32_t display_ms = 0;
#define DISPLAY_DELAY_MS 150
void display_sensor(){
  
  if(millis() - display_ms > DISPLAY_DELAY_MS){
    // Serial.println("spaghetti_state : " + String(spaghetti_state) + " spaghetti_calib: " + String(spaghetti_calib));
    // Serial.println("right_ticks: " + String(right_ticks) + " left_ticks: " + String(left_ticks));
    // Serial.println(batt_voltage);

    display.clearDisplay();
    display.setCursor(0, 16);
    display.print(batt_voltage);
    display.print(" " + String(imu_z_raw));
    display.println(" " + String(distance));
    display.println();

    display.print("     " + String(section_id) + "        ");
    display.print(finish_time);

    sprintf(display_buff,"        %4d          %4d %4d %4d %4d  %4d %4d %4d %4d ",(int)ir_array[4],(int)ir_array[3],(int)ir_array[2],(int)ir_array[1],(int)ir_array[0],(int)ir_array[8],(int)ir_array[7],(int)ir_array[6],(int)ir_array[5]);
    display.setCursor(0, 41);
    display.print(display_buff);
    display.display();
    display_ms = millis();
  }

}

//===================================================================================Sensor Handlers=================================================================================
void ir_handle(void * pram) { 
    while(1){
      ir_read();
      start_button_update();
      calib_button_update();
      batt_voltage_update();
      // boot_button_update();


      // For I2C Devices
      digitalWrite(PIN_RGB_LED,LOW);
      rgb_led_out();
      vTaskDelay(1);
    }
}


void odometry_handle(void * pram){

  while(1){
    imu_read();
    odometry_update();
    vTaskDelay(1);
  }
}


//=========================================================================================================IR Calibration
#define IR_CALIB_NB_SAMPLES 100
bool calib_started = false;

void ir_calib_exit(){
  calib_started = false;
  blue = 0;
  green = 0;
  red = 0;
  spaghetti_state = false;
  spaghetti_calib = false;
  fs_ir_claib_load();
  vTaskDelete(NULL);
}

TaskHandle_t task_ir_calib = NULL;
void ir_calib(void * pram){
    //============================================== Calibrating White
    spaghetti_state = false;
    while(spaghetti_calib && !spaghetti_state){
      display.clearDisplay();
      display.setCursor(16,32);
      display.println("Calibrate White");

      sprintf(display_buff,"        %4d          %4d %4d %4d %4d  %4d %4d %4d %4d ",ir_array_raw[4],ir_array_raw[3],ir_array_raw[2],ir_array_raw[1],ir_array_raw[0],ir_array_raw[8],ir_array_raw[7],ir_array_raw[6],ir_array_raw[5]);
      display.setCursor(0, 41);
      display.print(display_buff);
      display.display();
      vTaskDelay(1);
    }
    if(!spaghetti_calib) ir_calib_exit();


    spaghetti_state = false;
    display.clearDisplay();
    display.setCursor(16,32);
    display.println("Calibrating White");
    display.display();

    
    for(uint8_t i = 0; i < IR_NB;i++){
      float sum = 0;
      for(int j = 0; j < IR_CALIB_NB_SAMPLES;j++){
        sum = sum + ir_array_raw[i];
        vTaskDelay(1);
      }
      ir_array_white[i] =(int)(sum / IR_CALIB_NB_SAMPLES); 
    }



    //==============================================Calibrating Black
    while(spaghetti_calib && !spaghetti_state){
      display.clearDisplay();
      display.setCursor(16,32);
      display.println("Calibrate Black");

      sprintf(display_buff,"        %4d          %4d %4d %4d %4d  %4d %4d %4d %4d ",ir_array_raw[4],ir_array_raw[3],ir_array_raw[2],ir_array_raw[1],ir_array_raw[0],ir_array_raw[8],ir_array_raw[7],ir_array_raw[6],ir_array_raw[5]);
      display.setCursor(0, 41);
      display.print(display_buff);
      display.display();
      vTaskDelay(1);
    }

    if(!spaghetti_calib) ir_calib_exit();

    display.clearDisplay();
    display.setCursor(16,32);
    display.println("Calibrating Black");
    display.display();

    for(uint8_t i = 0; i < IR_NB;i++){
      float sum = 0;
      for(int j = 0; j < IR_CALIB_NB_SAMPLES;j++){
        sum = sum + ir_array_raw[i];
        vTaskDelay(1);
      }
      ir_array_black[i] =(int)(sum / IR_CALIB_NB_SAMPLES); 
    }
    fs_ir_claib_save();
    ir_calib_exit();
  }



//===============================================================================================Map Sections======================================================================
#define MAP_SECTION0_DISTANCE 40.0
void map_section0(){

  IMU_FORWARD_DUTY = 400;
  IMU_FORWARD_KP = 10.0;
  float imu_prev = imu_z_raw;
  float prev_distance = distance; 
  while(distance - prev_distance < 15){
    imu_forward(imu_prev);

    motors_update();
    vTaskDelay(1);
  }

  BLACK_THRESH = 1500;
  DISTANCE_THRESH = 0.4;
  detect_right_line_black = false;
  detect_left_line_black = false;
  count_right_line_black = 0;
  count_left_line_black = 0;

  IR_BASE_DUTY = 800;
  IR_KP = 0.1;
  IR_KD = 0.015;


  while(!(count_left_line_black >= 2)){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    detect_left_line_black = (ir_array[7]) < (BLACK_THRESH);
    ir_detect();
    motors_update();
    vTaskDelay(1);
  }
  prev_distance = distance;
  while((distance - prev_distance) < MAP_SECTION0_DISTANCE){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }



}
#define SECTION1_DISTANCE_0 10.0
#define SECTION1_DISTANCE_1 7.0

#define SECTION1_ANG_0 70
#define SECTION1_ANG_1 -70
#define SECTION1_ANG_2 180
void map_section1(){


  float BLACK_THRESH_LEFT;
  float BLACK_THRESH_RIGHT;
  float prev_imu;
  float prev_distance;


  BLACK_THRESH_LEFT = 3000;
  bool disable_right = false;

  IR_BASE_DUTY = 300;
  IR_KP = 0.2;
  IR_KD = 0.015;

  prev_imu = imu_z_raw;
  while(imu_z_raw - prev_imu < SECTION1_ANG_0){
    ir_error = (ir_array[5]) - (ir_array[3]);
    
    if(ir_array[6] < BLACK_THRESH_LEFT) {
      ir_error += (ir_array[6] - 4095);
    }
    if(ir_array[7] < BLACK_THRESH_LEFT) {
      ir_error += (ir_array[7] - 4095);
    }
    if(ir_array[8] < BLACK_THRESH_LEFT) {
      ir_error += (ir_array[8] - 4095); 
    }

    ir_controller();
    motors_update();

    vTaskDelay(1);
  }


  IR_BASE_DUTY = 300;
  IR_KP = 0.2;
  IR_KD = 0.015;

  prev_distance = distance;
  while((distance - prev_distance) < SECTION1_DISTANCE_0){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }



  BLACK_THRESH_RIGHT = 3000;
  prev_imu = imu_z_raw;
  while(imu_z_raw - prev_imu > SECTION1_ANG_1){
    ir_error = (ir_array[5]) - (ir_array[3]);
    
    if(ir_array[2] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[2]);
    }
    if(ir_array[1] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[1]);
    }
    if(ir_array[0] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[0]);
    }

    ir_controller();
    motors_update();

    vTaskDelay(1);
  }

  prev_distance = distance;
  while((distance - prev_distance) < SECTION1_DISTANCE_1){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }


  

  prev_imu = imu_z_raw;
  
  BLACK_THRESH_LEFT = 3000;
  BLACK_THRESH_RIGHT = 1300;

  IR_BASE_DUTY = 300;
  IR_KP = 0.20;
  IR_KD = 0.020;
  while(imu_z_raw - prev_imu < SECTION1_ANG_2){
    
    ir_error = (ir_array[5]) - (ir_array[3]);



    if(ir_array[6] < BLACK_THRESH_LEFT) {
      ir_error += (ir_array[6] - 4095) * 50;
    }
    else if(ir_array[2] < BLACK_THRESH_RIGHT){
      ir_error += (4095 - ir_array[2]) * 20;
    }

    if(ir_array[7] < BLACK_THRESH_LEFT) {
      ir_error += (ir_array[7] - 4095) * 50;
    }
    else if(ir_array[1] < BLACK_THRESH_RIGHT){
      ir_error += (4095 - ir_array[1]) * 20; 
    }

    if(ir_array[8] < BLACK_THRESH_LEFT) {
      ir_error += (ir_array[8] - 4095) * 50; 
    }
    else if(ir_array[0] < BLACK_THRESH_RIGHT){
      ir_error += (4095 - ir_array[0]) * 20; 
    }
    ir_controller();
    motors_update();
    vTaskDelay(1);
  }
  

}



#define MAP_SECTION2_DISTANCE_0 7.0
#define MAP_SECTION2_DISTANCE_1 5.0
#define MAP_SECTION2_DISTANCE_2 7.0
#define MAP_SECTION2_DISTANCE_3 5.0
#define MAP_SECTION2_DISTANCE_4 5.0

#define SECTION2_ANG_0 -50
#define SECTION2_ANG_1 70
#define SECTION2_ANG_2 -55
#define SECTION2_ANG_3 60
#define SECTION2_ANG_4 -50


void map_section2(){

  float BLACK_THRESH_LEFT;
  float BLACK_THRESH_RIGHT;
  float prev_imu;
  float prev_distance;

    
  BLACK_THRESH_RIGHT = 3000;
  BLACK_THRESH_LEFT = 1300;

  IR_BASE_DUTY = 300;
  IR_KP = 0.20;
  IR_KD = 0.015;


  prev_imu = imu_z_raw;
  while(imu_z_raw - prev_imu > SECTION2_ANG_0){
    ir_error = (ir_array[5]) - (ir_array[3]);
    if(ir_array[2] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[2]);
    }
    else if(ir_array[5] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[5] - 4095);
    }

    if(ir_array[1] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[1]);
    }
    else if(ir_array[7] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[7] - 4095); 
    }

    if(ir_array[0] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[0]); 
    }
    else if(ir_array[8] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[8] - 4095); 
    }
    ir_controller();
    motors_update();
    vTaskDelay(1);
  }

  prev_distance = distance;
  while((distance - prev_distance) < MAP_SECTION2_DISTANCE_0){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }



  prev_imu = imu_z_raw;
  while(imu_z_raw - prev_imu < SECTION2_ANG_1){
    ir_error = (ir_array[5]) - (ir_array[3]);
    if(ir_array[2] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[2]);
    }
    else if(ir_array[5] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[5] - 4095);
    }

    if(ir_array[1] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[1]);
    }
    else if(ir_array[7] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[7] - 4095); 
    }

    if(ir_array[0] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[0]); 
    }
    else if(ir_array[8] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[8] - 4095); 
    }
    ir_controller();
    motors_update();
    vTaskDelay(1);
  }

  prev_distance = distance;
  while((distance - prev_distance) < MAP_SECTION2_DISTANCE_1){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }




  prev_imu = imu_z_raw;
  while(imu_z_raw - prev_imu > SECTION2_ANG_2){
    ir_error = (ir_array[5]) - (ir_array[3]);
    if(ir_array[2] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[2]);
    }
    else if(ir_array[5] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[5] - 4095);
    }

    if(ir_array[1] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[1]);
    }
    else if(ir_array[7] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[7] - 4095); 
    }

    if(ir_array[0] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[0]); 
    }
    else if(ir_array[8] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[8] - 4095); 
    }
    ir_controller();
    motors_update();
    vTaskDelay(1);
  }

  prev_distance = distance;
  while((distance - prev_distance) < MAP_SECTION2_DISTANCE_2){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }


  
  prev_imu = imu_z_raw;
  while(imu_z_raw - prev_imu < SECTION2_ANG_3){
    ir_error = (ir_array[5]) - (ir_array[3]);
    if(ir_array[2] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[2]);
    }
    else if(ir_array[5] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[5] - 4095);
    }

    if(ir_array[1] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[1]);
    }
    else if(ir_array[7] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[7] - 4095); 
    }

    if(ir_array[0] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[0]); 
    }
    else if(ir_array[8] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[8] - 4095); 
    }
    ir_controller();
    motors_update();
    vTaskDelay(1);
  }

  prev_distance = distance;
  while((distance - prev_distance) < MAP_SECTION2_DISTANCE_3){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }



  prev_imu = imu_z_raw;
  while(imu_z_raw - prev_imu > SECTION2_ANG_4){
    ir_error = (ir_array[5]) - (ir_array[3]);
    if(ir_array[2] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[2]);
    }
    else if(ir_array[5] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[5] - 4095);
    }

    if(ir_array[1] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[1]);
    }
    else if(ir_array[7] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[7] - 4095); 
    }

    if(ir_array[0] < BLACK_THRESH_RIGHT) {
      ir_error += (4095 - ir_array[0]); 
    }
    else if(ir_array[8] < BLACK_THRESH_LEFT){
      ir_error += (ir_array[8] - 4095); 
    }
    ir_controller();
    motors_update();
    vTaskDelay(1);
  }

  
}

#define MAP_SECTION3_DISTANCE_0 80.0


#define SECTION3_ANG_0 70
#define SECTION3_ANG_1 60


void map_section3(){
  float BLACK_THRESH_LEFT;
  float BLACK_THRESH_RIGHT;
  float prev_imu;
  float prev_distance;



  WHITE_THRESH = 2500;
  IR_BASE_DUTY = 600;
  IR_KP = 0.20;
  IR_KD = 0.015;
  while(!(ir_array[4] > WHITE_THRESH)){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }


  prev_imu = imu_z_raw;


  IR_BASE_DUTY = 400;
  IR_KP = 0.07;
  IR_KD = 0.005;


  BLACK_THRESH = 1500;
  DISTANCE_THRESH = 6;
  precence_left_line_black = false;
  detect_left_line_black   = false;
  while(!precence_left_line_black){
        
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    detect_left_line_black = ir_array[4] < BLACK_THRESH;
    ir_detect();
    motors_update();
    vTaskDelay(1);
  }

  IR_BASE_DUTY = 600;
  IR_KP = 0.35;
  IR_KD = 0.020;
  
  while((imu_z_raw - prev_imu) < SECTION3_ANG_0){
    
    ir_error = (ir_array[5] + ir_array[6]) - (ir_array[3] + ir_array[2]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }


  prev_distance = distance;
  IR_BASE_DUTY = 900;
  IR_KP = 0.20;
  IR_KD = 0.015;

  while((distance - prev_distance) < MAP_SECTION3_DISTANCE_0){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }


  IR_BASE_DUTY = 400;
  IR_KP = 0.15;
  IR_KD = 0.01;

  prev_imu = imu_z;
  while((imu_z_raw - prev_imu) < SECTION3_ANG_1){
    
    ir_error = (ir_array[5] + ir_array[6]) - (ir_array[3] + ir_array[2]);
    ir_controller();


    motors_update();
    vTaskDelay(1);
  }



  IR_BASE_DUTY = 300;
  IR_KP = 0.15;
  IR_KD = 0.01;
  BLACK_THRESH = 1300;
  DISTANCE_THRESH = 2;
  detect_left_line_black   = false;
  detect_right_line_black   = false;
  precence_right_line_black = false;
  precence_left_line_black = false;
  while(!(precence_right_line_black && precence_left_line_black)){
    
    ir_error = (ir_array[5]) - (ir_array[3]);
    ir_controller();

    detect_left_line_black = ir_array[7]  < BLACK_THRESH;
    detect_right_line_black = ir_array[2] < BLACK_THRESH;

    ir_detect();


    motors_update();
    vTaskDelay(1);
  }

  
  
}

#define SECTION4_DISTANCE_0 10.0
void map_section4(){
  float prev_imu;
  float prev_distance;
  
  IR_BASE_DUTY = 250;
  IR_KP = 0.15;
  IR_KD = 0.01;
  WHITE_THRESH = 3500;
  DISTANCE_THRESH = 1;
  detect_left_line_white = 0;
  precence_left_line_white = false;
  
  prev_distance = distance;
  while(!(precence_left_line_white && distance - prev_distance > SECTION4_DISTANCE_0)){
    
    ir_error = (4095 - ir_array[5]) - (4095 - ir_array[3]);
    ir_controller();


    detect_left_line_white = (ir_array[8] + ir_array[7])  > (2 * WHITE_THRESH);

    ir_detect();

    motors_update();
    vTaskDelay(1);
  }
} 

#define SECTION5_DISTANCE_0 15.0
void map_section5(){
  float prev_imu;
  float prev_distance;
  


  IR_BASE_DUTY = 350;
  IR_KP = 0.15;
  IR_KD = 0.01;
  BLACK_THRESH = 1500;
  DISTANCE_THRESH = 2;
  detect_left_line_black = 0;
  detect_right_line_black = 0;
  precence_left_line_black = false;
  precence_right_line_black = false;

  while(!(precence_left_line_black && precence_right_line_black)){
    
    ir_error = (ir_array[5] + ir_array[6]) - (ir_array[3] + ir_array[2]);
    ir_controller();


    detect_left_line_black = (ir_array[6] + ir_array[7])  < (2 * BLACK_THRESH);
    detect_right_line_black = (ir_array[2] + ir_array[1]) < (2 * BLACK_THRESH);

    ir_detect();

    motors_update();
    vTaskDelay(1);
  }

  prev_imu = imu_z_raw;
  prev_distance = distance;


  
  IMU_FORWARD_DUTY = 350;
  prev_imu = imu_z_raw;
  prev_distance = distance; 
  while(distance - prev_distance < SECTION5_DISTANCE_0){
    imu_forward(prev_imu);

    motors_update();
    vTaskDelay(1);
  }

}


//=================================================================================================Global Map Handler=============================================================
void map_init(){
  imu_z = 0;
  imu_z_raw = 0;
  right_ticks = 0;
  left_ticks = 0;
  odometry_update();
}

bool map_started = false;
TaskHandle_t task_map_handler = NULL;
void map_handle(void * pram){
    map_init();
    start_time = millis();


    section_id = 0;
    map_section0();

    section_id = 1;
    map_section1();
    
    section_id = 2;
    map_section2();


    section_id = 3;
    map_section3();


    section_id = 4;
    map_section4();


    section_id = 5;
    map_section5();


    duty_left = 0;
    duty_right = 0;
    motors_update();
    finish_time = millis() - start_time;
    map_started = false;
    spaghetti_state = false;
    vTaskDelete(NULL);
}

//====================================================================================Main Program=====================================================================
void setup() {

  Serial.begin(115200);
  Wire.begin(PIN_SDA_0,PIN_SCL_0,400000);
  Wire1.begin(PIN_SDA_1,PIN_SCL_1,400000);
  WiFi.disconnect();
  
  WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK){
      Serial.println("esp_now Failed");
      ESP.restart();
  }

  esp_now_register_recv_cb(onRecv);


  pinout_init();
  motors_init();
  imu_init();
  odometry_init();

  fs_init();
  fs_ir_claib_load();
  imu_calibrate();

  display_init();
  rgb_led_init();

  xTaskCreatePinnedToCore(odometry_handle,"odometry_handle",4096,NULL,7,NULL,0);
  xTaskCreatePinnedToCore(ir_handle,"ir_handle",4096,NULL,6,NULL,0);

  spaghetti_state = false;
  spaghetti_calib = false;
  map_started = false;

}


// loop on Core 1 
void loop() {

  if(spaghetti_state && !spaghetti_calib) {
      if(!map_started){
          xTaskCreatePinnedToCore(map_handle,"map_handle",8192,NULL,6,&task_map_handler,1);
          map_started = true;
      }
  }
  else if(!spaghetti_state && spaghetti_calib){
      if(!calib_started){
          xTaskCreatePinnedToCore(ir_calib,"ir_calib",4096,NULL,4,&task_ir_calib,1);
          calib_started = true;
      }
  }
  else{
      if(map_started) {
          vTaskDelete(task_map_handler);
          map_started = false;
      } 
      duty_right = 0;
      duty_left = 0;
      motors_update();
  }



  if(!spaghetti_calib && !calib_started){
    display_sensor();
  }
}

