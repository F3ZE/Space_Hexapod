#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>
// 网络设置
const char* ssid = "ServoControl";  // WiFi名称
const char* password = "12345678";  // WiFi密码
// 创建PWM控制器实例
Adafruit_PWMServoDriver pwm_0 = Adafruit_PWMServoDriver(0x40);  
Adafruit_PWMServoDriver pwm_1 = Adafruit_PWMServoDriver(0x41);
// 定义6个电磁铁引脚
const int electromagnetPin[6] = {19, 18, 5, 27, 14, 12};  // 电磁铁控制引脚列表
const char* pinNames[6] = {"IO19", "IO18", "IO5", "IO27", "IO14", "IO12"};  // 引脚名称
bool electromagnetState[6] = {false, false, false, false, false, false};  // 初始状态都为关闭
// 舵机PWM参数
#define SERVO_MIN_PULSE  150  // 最小脉冲宽度
#define SERVO_MAX_PULSE  600  // 最大脉冲宽度
#define SERVO_FREQ       50   // 舵机PWM频率 (50Hz)
// 创建服务器对象
WebServer server(80);
// 舵机轴配置结构
struct ServoAxis {
  byte pwmController;  // 0 for pwm_0, 1 for pwm_1
  byte channel;        // PWM通道号
  String name;         // 轴名称
  int angle;           // 当前角度
};
// 定义所有舵机轴
ServoAxis servoAxes[] = {
  // 左前腿
  {1, 0, "Axis01 (servo15)", 90},  // servo15 -> Axis01 -> pwm_1.setPWM(0,x,x)
  {1, 1, "Axis11 (servo16)", 90},  // servo16 -> Axis11 -> pwm_1.setPWM(1,x,x)
  {1, 2, "Axis21 (servo17)", 90},  // servo17 -> Axis21 -> pwm_1.setPWM(2,x,x)
  
  // 左中腿
  {0, 4, "Axis02 (servo12)", 90},  // servo12 -> Axis02 -> pwm_0.setPWM(4,x,x)
  {0, 5, "Axis12 (servo13)", 90},  // servo13 -> Axis12 -> pwm_0.setPWM(5,x,x)
  {0, 6, "Axis22 (servo14)", 90},  // servo14 -> Axis22 -> pwm_0.setPWM(6,x,x)
  
  // 左后腿
  {0, 1, "Axis03 (servo9)", 90},   // servo9 -> Axis03 -> pwm_0.setPWM(1,x,x)
  {0, 2, "Axis13 (servo10)", 90},  // servo10 -> Axis13 -> pwm_0.setPWM(2,x,x)
  {0, 3, "Axis23 (servo11)", 90},  // servo11 -> Axis23 -> pwm_0.setPWM(3,x,x)
  
  // 右前腿
  {0, 9, "Axis06 (servo6)", 90},   // servo6 -> Axis06 -> pwm_0.setPWM(9,x,x)
  {0, 8, "Axis16 (servo7)", 90},   // servo7 -> Axis16 -> pwm_0.setPWM(8,x,x)
  {0, 0, "Axis26 (servo8)", 90},   // servo8 -> Axis26 -> pwm_0.setPWM(0,x,x)
  
  // 右中腿
  {0, 12, "Axis05 (servo3)", 90},  // servo3 -> Axis05 -> pwm_0.setPWM(12,x,x)
  {0, 11, "Axis15 (servo4)", 90},  // servo4 -> Axis15 -> pwm_0.setPWM(11,x,x)
  {0, 10, "Axis25 (servo5)", 90},  // servo5 -> Axis25 -> pwm_0.setPWM(10,x,x)
  
  // 右后腿
  {0, 15, "Axis04 (servo0)", 90},  // servo0 -> Axis04 -> pwm_0.setPWM(15,x,x)
  {0, 14, "Axis14 (servo1)", 90},  // servo1 -> Axis14 -> pwm_0.setPWM(14,x,x)
  {0, 13, "Axis24 (servo2)", 90},  // servo2 -> Axis24 -> pwm_0.setPWM(13,x,x)
  
  // 气瓶
  {1, 14, "Servo18", 90},          // servo18 -> pwm_1.setPWM(14,x,x)
  {1, 3, "Servo19", 90}            // servo19 -> pwm_1.setPWM(3,x,x)
};
const int NUM_SERVOS = sizeof(servoAxes) / sizeof(ServoAxis);
const int NUM_MAIN_AXES = 18;  // 主要的18个轴（不包括气瓶）
void setup() {
  Serial.begin(115200);
  
  // 设置所有电磁铁引脚
  for (int i = 0; i < 6; i++) {
    pinMode(electromagnetPin[i], OUTPUT);
    digitalWrite(electromagnetPin[i], LOW); // 初始状态为关闭
    Serial.print("初始化引脚 ");
    Serial.print(pinNames[i]);
    Serial.println(" 为输出模式");
  }
  
  // 初始化PWM控制器
  pwm_0.begin();
  pwm_1.begin();
  pwm_0.setPWMFreq(SERVO_FREQ);
  pwm_1.setPWMFreq(SERVO_FREQ);
  
  // 设置所有舵机初始位置
  for (int i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(i, 90);  // 设置为中间位置
  }
  
  // 设置接入点模式
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  // 设置服务器路由
  server.on("/", handleRoot);
  server.on("/setServoAngle", handleSetServoAngle);
  server.on("/getServoAngle", handleGetServoAngle);
  server.on("/setElectromagnet", handleSetElectromagnet);
  
  // 启动服务器
  server.begin();
  Serial.println("HTTP server started");
}
void loop() {
  server.handleClient();
}
// 将角度值(0-180)映射到舵机PWM值
int mapAngle(int uiAngle) {
  // 修正映射：当UI显示180度时，实际只发送相当于135度的PWM值
  int adjustedPwm;
  if (uiAngle > 135) {
    // 如果超过135度，就限制在最大PWM值
    adjustedPwm = map(135, 0, 180, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  } else {
    // 在0-135度范围内正常映射
    adjustedPwm = map(uiAngle, 0, 135, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  }
  return adjustedPwm;
}
// 设置舵机角度
void setServoAngle(int servoIndex, int angle) {
  if (servoIndex < 0 || servoIndex >= NUM_SERVOS) return;
  
  // 限制角度范围
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  
  ServoAxis &servo = servoAxes[servoIndex];
  servo.angle = angle;
  
  int pwmValue = mapAngle(angle);
  
  if (servo.pwmController == 0) {
    pwm_0.setPWM(servo.channel, 0, pwmValue);
  } else {
    pwm_1.setPWM(servo.channel, 0, pwmValue);
  }
  
  Serial.print("设置舵机 ");
  Serial.print(servo.name);
  Serial.print(" 角度: ");
  Serial.print(angle);
  Serial.print(" PWM: ");
  Serial.println(pwmValue);
}
// 处理根路径请求 - 发送HTML界面
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Servo Controller</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      background-color: #f0f0f0;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: white;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
      margin-top: 20px;
    }
    h1, h2 {
      text-align: center;
      color: #333;
    }
    .control-panel {
      margin: 20px 0;
    }
    .slider-container {
      margin: 15px 0;
    }
    .slider {
      width: 100%;
      height: 25px;
    }
    .angle-display {
      font-size: 1.2em;
      margin: 10px 0;
    }
    .axis-selection {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 15px;
      margin: 20px 0;
    }
    .axis-group {
      border: 1px solid #ddd;
      border-radius: 8px;
      padding: 15px;
      background-color: #f9f9f9;
    }
    .button {
      padding: 10px 15px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 1em;
      background-color: #4caf50;
      color: white;
      transition: background-color 0.3s;
    }
    .button:hover {
      background-color: #388e3c;
    }
    .button-container {
      display: flex;
      justify-content: center;
      gap: 10px;
      margin: 20px 0;
    }
    /* 开关样式 */
    .switch-container {
      display: flex;
      align-items: center;
      margin: 12px 0;
    }
    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
      margin: 0 10px;
    }
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider-switch {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #ccc;
      transition: .4s;
      border-radius: 34px;
    }
    .slider-switch:before {
      position: absolute;
      content: "";
      height: 26px;
      width: 26px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: .4s;
      border-radius: 50%;
    }
    input:checked + .slider-switch {
      background-color: #2196F3;
    }
    input:checked + .slider-switch:before {
      transform: translateX(26px);
    }
    /* 电磁铁控制区域样式 */
    .electromagnet-controls {
      background-color: #f9f9f9;
      border-radius: 8px;
      padding: 15px;
      margin: 20px 0;
      border: 1px solid #ddd;
    }
    .electromagnet-controls h3 {
      margin-top: 0;
      margin-bottom: 15px;
      color: #333;
      text-align: center;
    }
    .electromagnet-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }
    .pin-label {
      width: 70px;
      font-weight: bold;
    }
    .status-text {
      width: 80px;
      text-align: center;
    }
    .status {
      text-align: center;
      margin-top: 20px;
      font-style: italic;
      color: #555;
    }
    select {
      width: 100%;
      padding: 8px;
      border-radius: 4px;
      border: 1px solid #ddd;
      margin-bottom: 10px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Servo & Electromagnet Controller</h1>
    
    <!-- 轴选择和控制区域 -->
    <div class="control-panel">
      <h2>Servo Axis Control</h2>
      
      <div class="axis-selection">
        <div class="axis-group">
          <h3>Axis 1</h3>
          <select id="axisSelect0" onchange="updateAxisInfo(0)">
          )rawliteral";


          // 为第一个下拉菜单添加选项
  for (int i = 0; i < NUM_MAIN_AXES; i++) {
    html += "<option value=\"" + String(i) + "\">" + servoAxes[i].name + "</option>";
  }
  
  html += R"rawliteral(
          </select>
          <div class="angle-display">
            Angle: <span id="angleValue0">90</span>°
          </div>
          <input type="range" min="0" max="180" value="90" class="slider" id="angleSlider0" oninput="updateAngle(0, this.value)">
        </div>
        
        <div class="axis-group">
          <h3>Axis 2</h3>
          <select id="axisSelect1" onchange="updateAxisInfo(1)">
          )rawliteral";
          
  // 为第二个下拉菜单添加选项
  for (int i = 0; i < NUM_MAIN_AXES; i++) {
    html += "<option value=\"" + String(i) + "\">" + servoAxes[i].name + "</option>";
  }
  
  html += R"rawliteral(
          </select>
          <div class="angle-display">
            Angle: <span id="angleValue1">90</span>°
          </div>
          <input type="range" min="0" max="180" value="90" class="slider" id="angleSlider1" oninput="updateAngle(1, this.value)">
        </div>
        
        <div class="axis-group">
          <h3>Axis 3</h3>
          <select id="axisSelect2" onchange="updateAxisInfo(2)">
          )rawliteral";
          
  // 为第三个下拉菜单添加选项
  for (int i = 0; i < NUM_MAIN_AXES; i++) {
    html += "<option value=\"" + String(i) + "\">" + servoAxes[i].name + "</option>";
  }
  
  html += R"rawliteral(
          </select>
          <div class="angle-display">
            Angle: <span id="angleValue2">90</span>°
          </div>
          <input type="range" min="0" max="180" value="90" class="slider" id="angleSlider2" oninput="updateAngle(2, this.value)">
        </div>
        
        <div class="axis-group">
          <h3>Axis 4</h3>
          <select id="axisSelect3" onchange="updateAxisInfo(3)">
          )rawliteral";
          
  // 为第四个下拉菜单添加选项
  for (int i = 0; i < NUM_MAIN_AXES; i++) {
    html += "<option value=\"" + String(i) + "\">" + servoAxes[i].name + "</option>";
  }
  
  html += R"rawliteral(
          </select>
          <div class="angle-display">
            Angle: <span id="angleValue3">90</span>°
          </div>
          <input type="range" min="0" max="180" value="90" class="slider" id="angleSlider3" oninput="updateAngle(3, this.value)">
        </div>
      </div>
      
      <div class="button-container">
        <button class="button" onclick="setAllAxes(90)">Center All (90°)</button>
        <button class="button" onclick="setAllAxes(0)">Min All (0°)</button>
        <button class="button" onclick="setAllAxes(180)">Max All (180°)</button>
      </div>
    </div>
    
    <!-- 气瓶舵机控制 -->
    <div class="control-panel">
      <h2>Air Bottle Servos</h2>
      
      <div class="axis-group">
        <h3>Servo 18</h3>
        <div class="angle-display">
          Angle: <span id="angleValue18">90</span>°
        </div>
        <input type="range" min="0" max="180" value="90" class="slider" id="angleSlider18" oninput="updateSingleServo(18, this.value)">
      </div>
      
      <div class="axis-group">
        <h3>Servo 19</h3>
        <div class="angle-display">
          Angle: <span id="angleValue19">90</span>°
        </div>
        <input type="range" min="0" max="180" value="90" class="slider" id="angleSlider19" oninput="updateSingleServo(19, this.value)">
      </div>
    </div>
    
    <!-- 电磁铁控制区域 -->
    <div class="electromagnet-controls">
      <h3>Electromagnet Controls</h3>
      <div class="electromagnet-grid">
        <div class="switch-container">
          <span class="pin-label">IO19 (1):</span>
          <label class="switch">
            <input type="checkbox" id="electromagnetSwitch0" onchange="toggleElectromagnet(0)">
            <span class="slider-switch"></span>
          </label>
          <span id="electromagnetStatus0" class="status-text">OFF</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO18 (2):</span>
          <label class="switch">
            <input type="checkbox" id="electromagnetSwitch1" onchange="toggleElectromagnet(1)">
            <span class="slider-switch"></span>
          </label>
          <span id="electromagnetStatus1" class="status-text">OFF</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO5 (3):</span>
          <label class="switch">
            <input type="checkbox" id="electromagnetSwitch2" onchange="toggleElectromagnet(2)">
            <span class="slider-switch"></span>
          </label>
          <span id="electromagnetStatus2" class="status-text">OFF</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO27 (4):</span>
          <label class="switch">
            <input type="checkbox" id="electromagnetSwitch3" onchange="toggleElectromagnet(3)">
            <span class="slider-switch"></span>
          </label>
          <span id="electromagnetStatus3" class="status-text">OFF</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO14 (5):</span>
          <label class="switch">
            <input type="checkbox" id="electromagnetSwitch4" onchange="toggleElectromagnet(4)">
            <span class="slider-switch"></span>
          </label>
          <span id="electromagnetStatus4" class="status-text">OFF</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO12 (6):</span>
          <label class="switch">
            <input type="checkbox" id="electromagnetSwitch5" onchange="toggleElectromagnet(5)">
            <span class="slider-switch"></span>
          </label>
          <span id="electromagnetStatus5" class="status-text">OFF</span>
        </div>
      </div>
    </div>
    
    <div class="status">Status: <span id="connectionStatus">Connected</span></div>
  </div>
  
  <script>
    // 存储每个控制组选择的轴
    var selectedAxes = [0, 1, 2, 3]; // 默认选择前4个轴
    
    // 初始化页面
    window.onload = function() {
      // 设置默认选中值
      document.getElementById("axisSelect0").value = "0";
      document.getElementById("axisSelect1").value = "1";
      document.getElementById("axisSelect2").value = "2";
      document.getElementById("axisSelect3").value = "3";
      
      // 更新显示
      for (let i = 0; i < 4; i++) {
        updateAxisInfo(i);
      }
    }
    
    // 更新轴信息
    function updateAxisInfo(controlIndex) {
      const select = document.getElementById("axisSelect" + controlIndex);
      const axisIndex = parseInt(select.value);
      selectedAxes[controlIndex] = axisIndex;
      
      // 获取当前角度并更新滑块
      fetch('/getServoAngle?index=' + axisIndex)
        .then(response => response.json())
        .then(data => {
          document.getElementById("angleSlider" + controlIndex).value = data.angle;
          document.getElementById("angleValue" + controlIndex).innerText = data.angle;
        })
        .catch(error => {
          console.error('Error fetching angle:', error);
          document.getElementById("connectionStatus").innerHTML = "Connection Error";
        });
    }
    
    // 更新单个轴的角度
    function updateAngle(controlIndex, angle) {
      const axisIndex = selectedAxes[controlIndex];
      document.getElementById("angleValue" + controlIndex).innerText = angle;
      
      sendServoAngleRequest(axisIndex, angle);
    }
    
    // 更新单个舵机的角度（用于气瓶舵机）
    function updateSingleServo(servoIndex, angle) {
      document.getElementById("angleValue" + servoIndex).innerText = angle;
      
      sendServoAngleRequest(servoIndex, angle);
    }
    
    // 设置所有轴到指定角度
    function setAllAxes(angle) {
      // 更新4个轴控制
      for (let i = 0; i < 4; i++) {
        const axisIndex = selectedAxes[i];
        document.getElementById("angleSlider" + i).value = angle;
        document.getElementById("angleValue" + i).innerText = angle;
        
        sendServoAngleRequest(axisIndex, angle);
      }
    }
    
    // 发送舵机角度请求到服务器
    function sendServoAngleRequest(servoIndex, angle) {
      fetch('/setServoAngle?index=' + servoIndex + '&angle=' + angle)
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response was not ok');
          }
          document.getElementById("connectionStatus").innerHTML = "Connected";
          return response.text();
        })
        .catch(error => {
          document.getElementById("connectionStatus").innerHTML = "Connection Error";
          console.error('There was a problem with the fetch operation:', error);
        });
    }
    
    // 切换电磁铁状态
    function toggleElectromagnet(pinIndex) {
      var electromagnetSwitch = document.getElementById("electromagnetSwitch" + pinIndex);
      var electromagnetStatus = document.getElementById("electromagnetStatus" + pinIndex);
      
      fetch('/setElectromagnet?pin=' + pinIndex + '&state=' + (electromagnetSwitch.checked ? '1' : '0'))
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response was not ok');
          }
          electromagnetStatus.innerHTML = electromagnetSwitch.checked ? "ON" : "OFF";
          document.getElementById("connectionStatus").innerHTML = "Connected";
          return response.text();
        })
        .catch(error => {
          document.getElementById("connectionStatus").innerHTML = "Connection Error";
          console.error('There was a problem with the fetch operation:', error);
        });
    }
  </script>
</body>
</html>
  )rawliteral";
  
  server.send(200, "text/html; charset=UTF-8", html);
}
// 处理设置舵机角度请求
void handleSetServoAngle() {
  if (server.hasArg("index") && server.hasArg("angle")) {
    int servoIndex = server.arg("index").toInt();
    int angle = server.arg("angle").toInt();
    
    // 验证servoIndex是否在有效范围内
    if (servoIndex >= 0 && servoIndex < NUM_SERVOS) {
      setServoAngle(servoIndex, angle);
      
      server.send(200, "text/plain", "Servo " + String(servoIndex) + " angle set to: " + String(angle));
    } else {
      server.send(400, "text/plain", "Invalid servo index");
    }
  } else {
    server.send(400, "text/plain", "Missing index or angle parameter");
  }
}
// 处理获取舵机角度请求
void handleGetServoAngle() {
  if (server.hasArg("index")) {
    int servoIndex = server.arg("index").toInt();
    
    // 验证servoIndex是否在有效范围内
    if (servoIndex >= 0 && servoIndex < NUM_SERVOS) {
      String response = "{\"angle\":" + String(servoAxes[servoIndex].angle) + "}";
      server.send(200, "application/json", response);
    } else {
      server.send(400, "text/plain", "Invalid servo index");
    }
  } else {
    server.send(400, "text/plain", "Missing index parameter");
  }
}
// 处理设置电磁铁状态请求
void handleSetElectromagnet() {
  if (server.hasArg("pin") && server.hasArg("state")) {
    int pinIndex = server.arg("pin").toInt();
    String stateStr = server.arg("state");
    
    // 验证pinIndex是否在有效范围内
    if (pinIndex >= 0 && pinIndex < 6) {
      electromagnetState[pinIndex] = (stateStr == "1");
      
      digitalWrite(electromagnetPin[pinIndex], electromagnetState[pinIndex] ? HIGH : LOW);
      
      server.send(200, "text/plain", String(pinNames[pinIndex]) + " set to: " + (electromagnetState[pinIndex] ? "ON" : "OFF"));
      
      Serial.print(pinNames[pinIndex]);
      Serial.print(" state: ");
      Serial.println(electromagnetState[pinIndex] ? "ON" : "OFF");
    } else {
      server.send(400, "text/plain", "Invalid pin index");
    }
  } else {
    server.send(400, "text/plain", "Missing pin or state parameter");
  }
}