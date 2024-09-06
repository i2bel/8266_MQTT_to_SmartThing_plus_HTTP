
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);
    var font_mobsensors_2_voltage_value_txt = document.getElementById("mobsensors_2_voltage_value_txt");
    var font_sensors_0_battery_stat = document.getElementById("sensors_0_battery_stat");  
    var font_sensors_1_battery_stat = document.getElementById("sensors_1_battery_stat");  
    var font_sensors_2_battery_stat = document.getElementById("sensors_2_battery_stat");  
    var font_buderus_flame_status = document.getElementById("buderus_flame_status"); 
    var font_buderus_heating_status = document.getElementById("buderus_heating_status");     

    for (var i = 0; i < keys.length; i++){
       var key = keys[i];
      document.getElementById(key).innerHTML = myObj[key];      
      
if (key = "mobsensors_2_voltage_value_txt") { if  (myObj[key] == "Батарея в норме") {font_mobsensors_2_voltage_value_txt.style.color = "green";} else {font_mobsensors_2_voltage_value_txt.style.color = "red";}}
if (key = "sensors_0_battery_stat") { if  (myObj[key] == "Батарея в норме") {font_sensors_0_battery_stat.style.color = "green";} else {font_sensors_0_battery_stat.style.color = "red";}}
if (key = "sensors_1_battery_stat") { if  (myObj[key] == "Батарея в норме") {font_sensors_1_battery_stat.style.color = "green";} else {font_sensors_1_battery_stat.style.color = "red";}}
if (key = "sensors_2_battery_stat") { if  (myObj[key] == "Батарея в норме") {font_sensors_2_battery_stat.style.color = "green";} else {font_sensors_2_battery_stat.style.color = "red";}}
if (key = "buderus_flame_status") { if  (myObj[key] == "Вкл") {font_buderus_flame_status.style.color = "orange";} else {font_buderus_flame_status.style.color = "grey";}}
if (key = "buderus_heating_status") { if  (myObj[key] == "Вкл") {font_buderus_heating_status.style.color = "green";} else {font_buderus_heating_status.style.color = "grey";}}


 }
}