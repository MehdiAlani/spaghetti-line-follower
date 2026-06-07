// 26 i think last 

#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>




bool spaghetti_state = false;
bool spaghetti_calib = false;




#define PIN_BOOT 0


#define PIN_RF 42
#define PIN_RB 2
#define PIN_LB 1
#define PIN_LF 41

#define PIN_RGB_LED 48


#define PIN_ENCODER_RIGHT_A 45
#define PIN_ENCODER_RIGHT_B 45


#define PIN_ENCODER_LEFT_A 47
#define PIN_ENCODER_LEFT_B 47



#define PIN_SDA 4
#define PIN_SCL 5

#define PIN_MUX_SIG 14

#define PIN_MUX_S0 19
#define PIN_MUX_S1 20
#define PIN_MUX_S2 3
#define PIN_MUX_S3 48

void pinout_init(){

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
    bool state = mux_button_read(15); 
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
    bool state = mux_button_read(14); 
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




//==================================================================================================Motor Part==========================================================================================================================

#define RIGHT_CHANNEL_FORWARD 0
#define RIGHT_CHANNEL_BACKWARD 1
#define LEFT_CHANNEL_FORWARD 2
#define LEFT_CHANNEL_BACKWARD 3


#define MOTOR_RES 10
#define MOTOR_FREQ_KHZ 10
#define MAX_DUTY 1023


int duty_right = 0;
int duty_left = 0;

void motors_init(){
    ledcSetup(RIGHT_CHANNEL_FORWARD,MOTOR_FREQ_KHZ * 1000,MOTOR_RES);
    ledcAttachPin(PIN_RF,RIGHT_CHANNEL_FORWARD);
    ledcWrite(RIGHT_CHANNEL_FORWARD,0);


    ledcSetup(RIGHT_CHANNEL_BACKWARD,MOTOR_FREQ_KHZ * 1000,MOTOR_RES);
    ledcAttachPin(PIN_RB,RIGHT_CHANNEL_BACKWARD);
    ledcWrite(RIGHT_CHANNEL_BACKWARD,0);


    ledcSetup(LEFT_CHANNEL_FORWARD,MOTOR_FREQ_KHZ * 1000,MOTOR_RES);
    ledcAttachPin(PIN_LF,LEFT_CHANNEL_FORWARD);
    ledcWrite(LEFT_CHANNEL_FORWARD,0);


    ledcSetup(LEFT_CHANNEL_BACKWARD,MOTOR_FREQ_KHZ * 1000,MOTOR_RES);
    ledcAttachPin(PIN_LB,LEFT_CHANNEL_BACKWARD);
    ledcWrite(LEFT_CHANNEL_BACKWARD,0);


}


void motors_update(){
  if(duty_right >= 0){
    ledcWrite(RIGHT_CHANNEL_FORWARD,floor(duty_right,0,MAX_DUTY));
    ledcWrite(RIGHT_CHANNEL_BACKWARD ,0);
  }
  else{
    ledcWrite(RIGHT_CHANNEL_FORWARD,0);
    ledcWrite(RIGHT_CHANNEL_BACKWARD ,floor(abs(duty_right),0,MAX_DUTY));
  }


  if(duty_left >= 0){
    ledcWrite(LEFT_CHANNEL_FORWARD,floor(duty_left,0,MAX_DUTY));
    ledcWrite(LEFT_CHANNEL_BACKWARD ,0);
  }
  else{
    ledcWrite(LEFT_CHANNEL_FORWARD,0);
    ledcWrite(LEFT_CHANNEL_BACKWARD ,floor(abs(duty_left),0,MAX_DUTY));
  }


}


//======================================================================================================MPU SECTION=============================================================================================

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

float speed_z = 0;
float imu_z = 0;
int16_t gyro_z,calibrate_bias;
uint32_t mpu_time_us;


void imu_init(){

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(PWR_MGMT_1);          
  Wire.write(0);
  Wire.endTransmission(true);


  Wire.beginTransmission(MPU_ADDR);
  Wire.write(GYRO_CONFIG);          
  Wire.write(GYRO_250);           
  Wire.endTransmission(true);

}



float imu_sum = 0;

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

        speed_z = (gyro_z - calibrate_bias) / GYRO_250_COFF;

        imu_sum += speed_z * (diff) / 1000000.0;
        imu_z += speed_z * (diff) / 1000000.0;
        if(imu_z > 360) imu_z = 0;
        if(imu_z < 0) imu_z = 360;
    }
}

#define NB_SAMPLES 1000
void imu_calibrate(){
  int32_t sum = 0;
  calibrate_bias = 0;
  for(int i = 0; i < NB_SAMPLES; i++){
    imu_read();
    sum += gyro_z;
    delayMicroseconds(IMU_SAMPLE_TIME_US);
  }
  calibrate_bias = sum / NB_SAMPLES;
  imu_z = 0;
  imu_sum = 0;
}


//=============================================================================================Odemtry Part=============================================================================================================

#define MIN_TIME_US 4000

volatile uint32_t right_ticks = 0;
uint32_t right_tmr_us = 0;
uint32_t right_diff_us = 0;


void IRAM_ATTR ENCODER_ISR_RIGHT(){
  right_ticks++;
}



volatile uint32_t left_ticks = 0;
uint32_t left_tmr_us = 0;
uint32_t left_diff_us = 0;

void IRAM_ATTR ENCODER_ISR_LEFT(){
  left_ticks++;
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
  imu_z = 0;
  imu_sum = 0;
  imu_init();

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_RIGHT_A), ENCODER_ISR_RIGHT, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_LEFT_A), ENCODER_ISR_LEFT, RISING);
}



void odemtry_handle(){
    right_distance = (WHEEL_RADUIS_CM * 2 * PI) * (right_ticks / ENCODER_STEPS_PER_REV);
    left_distance = (WHEEL_RADUIS_CM * 2 * PI) * (left_ticks / ENCODER_STEPS_PER_REV);
    distance = (right_distance + left_distance) / 2;

}



//================================================================================================IR Part=============================================================================================




#define IR_NB 9
float ir_array[IR_NB]; 
void ir_read(){
    for(uint8_t i = 0; i < IR_NB; i++){
        mux_select(i);
        ir_array[i] = analogRead(PIN_MUX_SIG);
    }
}




//=======================================================================================Sensor Part========================================================================================

void sensor_read(void * pram) { 
    while(1){
        ir_read();
        imu_read();
        start_button_update();
        // boot_button_update();
        vTaskDelay(1);
    }
}


//========================================================================================Controllers=====================================================================================

#define PID_DELAY_US 2000

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



// Turn Angle

float angle_diff(float angle1 , float angle2){
    float diff1 = abs(angle1 - angle2);
    float diff2 = 360 - diff1;
    if(diff2 <= diff1) return diff2;
    return diff1;
}

#define ANGLE_EPSELON 23
#define TURN_BASE_DUTY 300
void turn_angle(float angle){

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



//==========================================================================================WebServer==================================================================================================================
#define JSON_SIZE 512
StaticJsonDocument<JSON_SIZE> doc_send;
JsonArray ir_array_json;
char json_buff_send[JSON_SIZE];
String log_msg;


StaticJsonDocument<JSON_SIZE> doc_recv;
char json_buff_recv[JSON_SIZE];


#define SSID "spaghetti"
#define PASS "uJZWM9FRh4N1JJpQ"


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");



void ws_event(AsyncWebSocket *server, AsyncWebSocketClient *client,AwsEventType type, void *arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){
        Serial.println("Client Connected Bro " + String(client->id()));
    }



    if(type == WS_EVT_DATA){
        String jsonString = "";
        for (size_t i = 0; i < len; i++) {
            jsonString += (char)data[i];
        }
        deserializeJson(doc_recv, jsonString);
        spaghetti_state = doc_recv["start"];
        spaghetti_calib = doc_recv["pause"];
    }


    if(type == WS_EVT_DISCONNECT){
        Serial.println("Client Disconnected Bro ");
    }

}


void init_webserver(){
    doc_send["boot_time"];
    doc_send["cpu_temp"];
    doc_send["imu_z"];
    doc_send["distance"];
    doc_send["start"];
    doc_send["pause"];
    doc_send["log"];
    ir_array_json = doc_send.createNestedArray("ir_array");

    for(int i = 0; i < IR_NB;i++){
        ir_array_json.add(0);
    }


    WiFi.softAP(SSID,PASS);

    LittleFS.begin();
    Serial.println("Listing LittleFS files:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.println("File Name: " + String(file.name()) + "\t Size: " + String(file.size()));
        file = root.openNextFile();
    }



    server.serveStatic("/", LittleFS, "/");
    // INIT WEBSOCKET

    ws.onEvent(ws_event);
    server.addHandler(&ws);

    server.begin();
}

void update_json(){
    doc_send["boot_time"] = millis() / 1000;
    doc_send["cpu_temp"] = temperatureRead();
    doc_send["imu_z"] = imu_z;
    doc_send["distance"] = distance;
    doc_send["start"] = spaghetti_state;
    doc_send["calib"] = spaghetti_calib;
    doc_send["log"] = log_msg.c_str();

    for(int i = 0; i < IR_NB ;i++){
        ir_array_json[i] = ir_array[i];
    }
    serializeJson(doc_send,json_buff_send);
}
//==========================================================================================Map Handler==================================================================================================================


int nb_turns = 0;
float angle_turn = 0;
float angle_ref = 0;



void map_section4_count_turns(){
    if(angle_diff(imu_z,angle_ref) > angle_turn){
        nb_turns++;
        angle_ref = imu_z;
    }
} 



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

void map_state(){
    odemtry_handle();

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


#define MOVE_FORWARD_KP 10
void move_forward(int base_duty,float move_dist,float move_angle){
    map_state();
    float prev_dist = distance;
    while(abs(distance - prev_dist) < move_dist){
        duty_left  =  base_duty + MOVE_FORWARD_KP * (imu_sum - move_angle);
        duty_right =  base_duty - MOVE_FORWARD_KP * (imu_sum - move_angle);
        motors_update();
        map_state();
        vTaskDelay(1);
    }   
}


//===========================================================================================Map Sections============================================================================================================================================

//===================================================Section0 
void map_section0(){
    move_forward(300,20,imu_sum);



    count_left_line_black = 0;
    count_right_line_black = 0;
    BLACK_THRESH = 1200;
    DISTANCE_THRESH = 1;

    IR_BASE_DUTY = 250;
    IR_KP = 0.15;
    IR_KD = 0.003;
    while(count_right_line_black < 2 && count_left_line_black < 2){
        ir_error = (ir_array[5]) - (ir_array[3]);
        ir_controller();
        motors_update();


        detect_right_line_black = ir_array[1] + ir_array[2] <= (BLACK_THRESH * 2);
        detect_left_line_black  = ir_array[6] + ir_array[7]  <= (BLACK_THRESH * 2);
        map_state();
        vTaskDelay(1);
    }

    move_forward(300,20,imu_sum);

}


//=========================================================Section1 
#define SECTION1_ANGLE_TURN 40


void map_section1(){
    IR_BASE_DUTY = 250;
    IR_KP = 0.15;
    IR_KD = 0.01;
    map_state();
    precence_right_line_black = false;
    precence_left_line_black = false;
    BLACK_THRESH = 1300;
    WHITE_THRESH = 3500;
    DISTANCE_THRESH = 1;
    bool black_flag = false;
    float prev_distance = distance;
    float ref_angle = imu_z;

    while(abs(distance - prev_distance) < 50){
        ir_error = (ir_array[5]) - (ir_array[3]);
        black_flag = false;


        if(ir_array[2] < 1400){
            ir_error += (WHITE_THRESH - ir_array[2]) * 5;
            black_flag = true;
        }
        else if(ir_array[6] < 1400 && !black_flag){
            ir_error += (ir_array[6] - WHITE_THRESH) * 2;
        }


        if(ir_array[1] < 1400){
            ir_error += (WHITE_THRESH - ir_array[1]) * 5;
            prev_distance = distance;
        }
        else if(ir_array[7] < 1400 && !black_flag){
            ir_error += (ir_array[7] - WHITE_THRESH) * 2;
        }


        if(ir_array[0] < 1400){
            ir_error += (WHITE_THRESH - ir_array[0]) * 5;
        }
        else if(ir_array[8] < 1400 && !black_flag){
            ir_error += (ir_array[8] - WHITE_THRESH) * 2;
        }

        ir_controller();
        duty_right = floor(duty_right,-700,400);
        duty_left = floor(duty_left,-400,700);
        motors_update();

        if(angle_diff(imu_z,ref_angle) > SECTION1_ANGLE_TURN){
            ref_angle = imu_z;
            prev_distance = distance;
        }
        map_state();
        vTaskDelay(1);
    }
        
}


//=========================================================Section2


void map_section2(){

    count_left_line_black = 0;
    count_right_line_black = 0;
    BLACK_THRESH = 1400;
    DISTANCE_THRESH = 1;

    IR_BASE_DUTY = 400;
    IR_KP = 0.3;
    IR_KD = 0.015;
    precence_left_line_black = false;
    precence_right_line_black = false;
    while(count_left_line_black < 2){
        ir_error = (ir_array[5]) - (ir_array[3]);
        ir_controller();
        motors_update();


        detect_right_line_black = (ir_array[0] + ir_array[1]) <= (BLACK_THRESH * 2);
        detect_left_line_black  = (ir_array[8] + ir_array[7])  <= (BLACK_THRESH * 2);
        map_state();
        vTaskDelay(1);
    }


    count_left_line_black = 0;
    count_right_line_black = 0;
    precence_left_line_black = false;
    precence_right_line_black = false;
    BLACK_THRESH = 1200;
    DISTANCE_THRESH = 0;

    IR_BASE_DUTY = 200;
    IR_KP = 0.15;
    IR_KD = 0.003;
    WHITE_THRESH = 3000;
    angle_ref = imu_z;
    angle_turn = 50;
    nb_turns = 0;
    while(!(precence_left_line_black && precence_right_line_black)){
        ir_error = (ir_array[5] + ir_array[6]) - (ir_array[3] + ir_array[2]);
        ir_controller();
        motors_update();

        duty_right = floor(duty_right,-400,400);
        duty_left = floor(duty_left,-400,400);
        
        detect_right_line_black = ir_array[0] + ir_array[1] <= (BLACK_THRESH * 2);
        detect_left_line_black  = ir_array[8] + ir_array[7]  <= (BLACK_THRESH * 2);
        map_state();
        vTaskDelay(1);
    }

}


//=========================================================Section3
#define MAP_SECTION3 10

void  map_section3(){

    
    IR_BASE_DUTY = 200;
    IR_KP = 0.3;
    IR_KD = 0.007;



    WHITE_THRESH = 2500;
    BLACK_THRESH = 1400;
    count_left_line_white = 0;
    count_right_line_white = 0;
    precence_right_line_white = false;
    precence_left_line_white = false;
    DISTANCE_THRESH = 2;

    float prev_distance = distance;
    while(1){
        ir_error = (4095 - ir_array[5] + 4095 - ir_array[6]) - (4095 - ir_array[3] + 4095 - ir_array[2]);

        ir_controller();
        duty_right = floor(duty_right,-400,700);
        duty_left = floor(duty_left,-700,400);


        motors_update();   
        if(abs(distance - prev_distance) > MAP_SECTION3 && detect_left_line_white){
            break;
        }

        detect_right_line_white = ir_array[0] + ir_array[1]  >= (WHITE_THRESH * 2);
        detect_left_line_white  = ir_array[7] + ir_array[8]  >= (WHITE_THRESH * 2);
        map_state();
        vTaskDelay(1);
    }
    turn_angle(50);

    count_left_line_white = 0;
    count_right_line_white = 0;
    precence_right_line_white = false;
    precence_left_line_white = false;
    WHITE_THRESH = 2500;
    BLACK_THRESH = 1500;
    DISTANCE_THRESH = 2;


    while(!precence_right_line_white){
        ir_error = (4095 - ir_array[5]) - (4095 - ir_array[3]);

        ir_controller();
        motors_update();   

        duty_right = floor(duty_right,-400,400);
        duty_left = floor(duty_left,-400,400);


        detect_right_line_white = ir_array[1]   >= (WHITE_THRESH);
        detect_left_line_white  = ir_array[7]   >= (WHITE_THRESH);

        map_state();
        vTaskDelay(1);
    }
    

    turn_angle(-50);



    // count_left_line_white = 0;
    // count_right_line_white = 0;
    // precence_right_line_white = false;
    // precence_left_line_white = false;
    // WHITE_THRESH = 2500;
    // BLACK_THRESH = 1500;
    // DISTANCE_THRESH = 2;

    prev_distance = distance;
    while(distance - prev_distance < 30){
        ir_error = (4095 - ir_array[5]) - (4095 - ir_array[3]);

        ir_controller();
        motors_update();   
        
        duty_right = floor(duty_right,-400,400);
        duty_left = floor(duty_left,-400,400);


        detect_right_line_white = ir_array[1]   >= (WHITE_THRESH);
        detect_left_line_white  = ir_array[7]   >= (WHITE_THRESH);

        map_state();
        vTaskDelay(1);
    }


    count_left_line_white = 0;
    count_right_line_white = 0;
    precence_right_line_white = false;
    precence_left_line_white = false;
    WHITE_THRESH = 2500;
    BLACK_THRESH = 1500;
    DISTANCE_THRESH = 2;


        
    IR_BASE_DUTY = 200;
    IR_KP = 0.3;
    IR_KD = 0.007;

    while(!precence_right_line_white){
        ir_error = (4095 - ir_array[5]) - (4095 - ir_array[3]);

        ir_controller();
        duty_right = floor(duty_right,-300,300);
        duty_left = floor(duty_left,-300,300);

        motors_update();   

        detect_right_line_white = ir_array[1]  >= (WHITE_THRESH);
        detect_left_line_white  = ir_array[7]  >= (WHITE_THRESH);

        map_state();
        vTaskDelay(1);
    }

    move_forward(250,10,imu_sum);
    turn_angle(-90);


    
    while(!detect_left_line_white){
        ir_error = (4095 - ir_array[5]) - (4095 - ir_array[3]);

        ir_controller();
        duty_right = floor(duty_right,-300,300);
        duty_left = floor(duty_left,-300,300);
        motors_update();   


        detect_right_line_white = ir_array[1]  >= (WHITE_THRESH);
        detect_left_line_white  = ir_array[7]  >= (WHITE_THRESH);

        map_state();
        vTaskDelay(1);
    }
    move_forward(250,15,imu_sum);
    turn_angle(70);

    count_left_line_white = 0;
    count_right_line_white = 0;
    DISTANCE_THRESH = 1;
    WHITE_THRESH = 2000;

    while(!precence_left_line_white){
        ir_error = (4095 - ir_array[5]) - (4095 - ir_array[3]);
        ir_controller();

        duty_right = floor(duty_right,-300,300);
        duty_left = floor(duty_left,-300,300);
        motors_update();   


        detect_right_line_white = ir_array[1]  >= (WHITE_THRESH);
        detect_left_line_white  = ir_array[7]  >= (WHITE_THRESH);

        map_state();
        vTaskDelay(1);
    }
    
}

//=========================================================Section4
#define SECTION4_ANGLE_TURN 100.0
#define SECTION4_DISTANCE_0 20
#define SECTION4_DISTANCE_1 10
void map_section4(){
    IR_BASE_DUTY = 300;
    IR_KP = 0.15;
    IR_KD = 0.003;

    precence_left_line_black  = false;
    precence_right_line_black = false;
    count_left_line_black = 0;
    count_right_line_black = 0;
    BLACK_THRESH = 1400;
    DISTANCE_THRESH = 1;

    imu_z = 0;
    float ref_angle = imu_z;

    int turn_count = 0;


    while(turn_count < 1){
        ir_error = (ir_array[5]) - (ir_array[3]);

        if((ir_array[7] + ir_array[8]) < (BLACK_THRESH * 2)){
                turn_angle(40);
                
        }

        if(angle_diff(imu_z,ref_angle) > SECTION4_ANGLE_TURN){
            ref_angle = imu_z;
            turn_count++;
        }


        ir_controller();
        duty_right = floor(duty_right,-500,500);
        duty_left = floor(duty_left,-500,500);

        motors_update();


        detect_right_line_black = (ir_array[2] + ir_array[1])  <= (BLACK_THRESH * 2);
        detect_left_line_black  = (ir_array[6] + ir_array[7])  <= (BLACK_THRESH * 2);
        map_state();
        vTaskDelay(1);
    }

    detect_left_line_black = false;
    BLACK_THRESH = 1400;
    float prev_distance = distance;
    while(1){
        ir_error = (ir_array[5]) - (ir_array[3]);

        ir_controller();

        motors_update();

        detect_right_line_black = (ir_array[1])  <= (BLACK_THRESH);
        detect_left_line_black  = (ir_array[7])  <= (BLACK_THRESH);
        map_state();

        if(abs(distance - prev_distance) > SECTION4_DISTANCE_0 && detect_left_line_black){
            break;
        }

        vTaskDelay(1);
    }
    turn_angle(40);

    detect_right_line_black = false;
    detect_left_line_black = false;
    BLACK_THRESH = 1400;
    prev_distance = distance;
    while(!(detect_right_line_black && detect_left_line_black)){
        ir_error = (ir_array[5]) - (ir_array[3]);

        ir_controller();

        motors_update();

        detect_right_line_black = (ir_array[1])  <= (BLACK_THRESH);
        detect_left_line_black  = (ir_array[7])  <= (BLACK_THRESH);
        map_state();

        if((ir_array[1] + ir_array[0]) < (BLACK_THRESH * 2)){
            turn_angle(-40);
        }

        vTaskDelay(1);
    }


    move_forward(300,10,imu_sum);

}

//=================================================================================================Global Map Handler=============================================================


bool map_started = false;
int section_id = 0;
uint32_t start_time = 0, finish_time = 0;
TaskHandle_t task_map_handler = NULL;
void map_handle(void * pram){
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


    duty_left = 0;
    duty_right = 0;
    motors_update();

    finish_time = millis() - start_time;
    map_started = false;
    spaghetti_state = false;
    vTaskDelete(NULL);
}



//============================================================================================Main Program=============================================================================================================
uint32_t time_ms = 0;

void setup(){
    Serial.begin(115200);
    // init_webserver();
    
    
    Wire.begin(PIN_SDA,PIN_SCL,400000);
    pinout_init();
    motors_init();
    duty_right = 0;
    duty_left = 0;

    odemtry_init();
    imu_calibrate();

    xTaskCreatePinnedToCore(sensor_read,"sensor_read",4096,NULL,5,NULL,0);

    boot_debounce_ms = millis();
    start_debounce_ms = millis();
    time_ms = millis();

}

uint16_t delay_ms = 100;
void loop(){

    if(millis() - time_ms > delay_ms){
        // if(ws.count() > 0){
        //     log_msg = "duty_right : " + String(duty_right) + " duty_left : " + String(duty_left) + "\n";
        //     log_msg += "ir_pid : " + String(ir_pid) + " ir_p : " + String(ir_p) + " ir_i : " + String(ir_i) + " ir_d : " + String(ir_d)+ "\n";
        //     log_msg += "count_left_line_black : " + String(count_left_line_black) + " count_right_line_black: " + String(count_right_line_black)+ "\n";
        //     log_msg += "section_id: " + String(section_id) + " finish_time: " + String(finish_time) + " ms"; 


        //     update_json();
        //     ws.textAll(json_buff_send);
        // }
        time_ms = millis();
    }
    if(spaghetti_state) {
        if(!map_started){
            xTaskCreatePinnedToCore(map_handle,"map_handle",4096,NULL,4,&task_map_handler,0);
            map_started = true;
        }
    }
    else{
        if(map_started) {
            vTaskDelete(task_map_handler);
            map_started = false;
        } 
        motors_init();
        duty_right = 0;
        duty_left = 0;
        distance = 0;
    }

}