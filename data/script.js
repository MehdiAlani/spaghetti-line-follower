const IR_NB = 9;

let json_recv;
let json_send = {
    start: false,
    pause: false
};


let web_pause_butt = false;
let web_start_butt = false;


const startBtn = document.getElementById("start");
const pauseBtn = document.getElementById("pause");


startBtn.addEventListener("click", () => {
    web_start_butt = !web_start_butt;
    handle_send();
    ws.send(JSON.stringify(json_send));
});

pauseBtn.addEventListener("click", () => {
    web_pause_butt = !web_pause_butt;
    handle_send();
    ws.send(JSON.stringify(json_send));
});


function handle_send(){
    json_send.start = web_start_butt;
    json_send.pause = web_pause_butt;
}

function handle_recv(){
    for(let i = 0; i < IR_NB;i++){
    const element = document.getElementById(`IR_${IR_NB - 1 - i}`);
    val = json_recv.ir_array[i];
    element.textContent = val;
    val = Math.floor(val * 255 / 4095);
    element.style.background = `rgb(${val},${val},${val})`;
   }
   element = document.getElementById("boot_time");
   element.textContent = json_recv.boot_time;

   element = document.getElementById("cpu_temp");
   element.textContent = json_recv.cpu_temp;

   element = document.getElementById("imu_z");
   element.textContent = json_recv.imu_z;

   element = document.getElementById("distance");
   element.textContent = json_recv.distance;


   element = document.getElementById("start");
   web_start_butt = json_recv.start;
   if(web_start_butt){
    element.textContent = "Stop";
    element.style.backgroundColor = "red";
   }
   else{
    element.textContent = "Start";
    element.style.backgroundColor = "green";
   }


   element = document.getElementById("pause");
   web_pause_butt = json_recv.pause;
   if(web_pause_butt){
    element.textContent = "Resume";
    element.style.backgroundColor = "red";
   }
   else{
    element.textContent = "Pause";
    element.style.backgroundColor = "green";
   }

   element = document.getElementById("log");
   element.textContent = json_recv.log;

}

let val;
let ws = new WebSocket("ws://" + window.location.hostname + "/ws");



ws.onopen = () => {
};

ws.onmessage = (event) => {
    json_recv = JSON.parse(event.data);
    handle_recv(); 
};

ws.onclose = () => {
    console.log("WebSocket closed");
};