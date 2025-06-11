#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>
// 网络设置
const char* ssid = "ServoControl";  // WiFi名称
const char* password = "12345678";  // WiFi密码
// 创建PWM控制器实例
Adafruit_PWMServoDriver pwm_0 = Adafruit_PWMServoDriver(0x40);  
Adafruit_PWMServoDriver pwm_1 = Adafruit_PWMServoDriver(0x41);
// 定义6个使能引脚
const int enablePin[6] = {19, 18, 5, 27, 14, 12};  // 使能控制引脚列表
const char* pinNames[6] = {"IO19", "IO18","IO5", "IO27", "IO14", "IO12"};  // 引脚名称
bool enableState[6] = {false, false, false, false, false, false};  // 初始状态都为禁用
// 舵机参数
const int servoChannel = 0; // 舵机连接的PWM通道
// 舵机PWM参数
#define SERVO_MIN_PULSE  150  // 最小脉冲宽度
#define SERVO_MAX_PULSE  600  // 最大脉冲宽度
#define SERVO_FREQ       50   // 舵机PWM频率 (50Hz)
// 创建服务器对象
WebServer server(80);
// 当前舵机速度值
int currentSpeed = 0;
void setup() {
  Serial.begin(115200);
  
  // 设置所有GPIO引脚
  for (int i = 0; i < 6; i++) {
    pinMode(enablePin[i], OUTPUT);
    digitalWrite(enablePin[i], LOW); // 初始状态为禁用
    Serial.print("初始化引脚 ");
    Serial.print(pinNames[i]);
    Serial.println(" 为输出模式");
  }
  
  // 初始化PWM控制器
  pwm_0.begin();
  pwm_1.begin();
  pwm_0.setPWMFreq(SERVO_FREQ);
  pwm_1.setPWMFreq(SERVO_FREQ);
  
  // 设置舵机初始位置为停止
  pwm_0.setPWM(servoChannel, 0, mapSpeed(0));
  
  // 设置接入点模式
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  // 设置服务器路由
  server.on("/", handleRoot);
  server.on("/setSpeed", handleSetSpeed);
  server.on("/setEnable", handleSetEnable); // 使能控制路由
  
  // 启动服务器
  server.begin();
  Serial.println("HTTP server started");
}
void loop() {
  server.handleClient();
}
// 将速度值(-100到100)映射到舵机PWM值
int mapSpeed(int speed) {
  int pwmValue;
  
  if (speed == 0) {
    pwmValue = (SERVO_MIN_PULSE + SERVO_MAX_PULSE) / 2; // 中间位置停止
  } else if (speed < 0) {
    // 负值映射到停止位置到最小脉冲
    pwmValue = map(speed, 0, -100, (SERVO_MIN_PULSE + SERVO_MAX_PULSE) / 2, SERVO_MIN_PULSE);
  } else {
    // 正值映射到停止位置到最大脉冲
    pwmValue = map(speed, 0, 100, (SERVO_MIN_PULSE + SERVO_MAX_PULSE) / 2, SERVO_MAX_PULSE);
  }
  
  return pwmValue;
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
      max-width: 600px;
      margin: 0 auto;
      padding: 20px;
      background-color: white;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
      margin-top: 20px;
    }
    h1 {
      text-align: center;
      color: #333;
    }
    .control-panel {
      margin: 20px 0;
      text-align: center;
    }
    .slider-container {
      margin: 20px 0;
    }
    .slider {
      width: 100%;
      height: 25px;
    }
    .speed-display {
      font-size: 1.5em;
      font-weight: bold;
      margin: 15px 0;
    }
    .direction {
      margin: 10px 0;
      font-size: 1.2em;
      color: #555;
    }
    .button-container {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 10px;
      margin: 20px 0;
    }
    .button {
      padding: 10px 15px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 1em;
      transition: background-color 0.3s;
    }
    .stop-btn {
      background-color: #ff5252;
      color: white;
      font-weight: bold;
      padding: 10px 30px;
    }
    .stop-btn:hover {
      background-color: #ff0000;
    }
    .preset-btn {
      background-color: #4caf50;
      color: white;
    }
    .preset-btn:hover {
      background-color: #388e3c;
    }
    .status {
      text-align: center;
      margin-top: 20px;
      font-style: italic;
      color: #555;
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
    /* 引脚控制区域样式 */
    .pin-controls {
      background-color: #f9f9f9;
      border-radius: 8px;
      padding: 15px;
      margin: 20px 0;
      border: 1px solid #ddd;
    }
    .pin-controls h3 {
      margin-top: 0;
      margin-bottom: 15px;
      color: #333;
      text-align: center;
    }
    .pin-grid {
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
  </style>
</head>
<body>
  <div class="container">
    <h1>Servo Controller</h1>
    
    <!-- IO引脚控制区域 -->
    <div class="pin-controls">
      <h3>GPIO Controls</h3>
      <div class="pin-grid">
        <div class="switch-container">
          <span class="pin-label">IO33:</span>
          <label class="switch">
            <input type="checkbox" id="enableSwitch0" onchange="toggleEnable(0)">
            <span class="slider-switch"></span>
          </label>
          <span id="enableStatus0" class="status-text">Disabled</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO32:</span>
          <label class="switch">
            <input type="checkbox" id="enableSwitch1" onchange="toggleEnable(1)">
            <span class="slider-switch"></span>
          </label>
          <span id="enableStatus1" class="status-text">Disabled</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO35:</span>
          <label class="switch">
            <input type="checkbox" id="enableSwitch2" onchange="toggleEnable(2)">
            <span class="slider-switch"></span>
          </label>
          <span id="enableStatus2" class="status-text">Disabled</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO34:</span>
          <label class="switch">
            <input type="checkbox" id="enableSwitch3" onchange="toggleEnable(3)">
            <span class="slider-switch"></span>
          </label>
          <span id="enableStatus3" class="status-text">Disabled</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO39:</span>
          <label class="switch">
            <input type="checkbox" id="enableSwitch4" onchange="toggleEnable(4)">
            <span class="slider-switch"></span>
          </label>
          <span id="enableStatus4" class="status-text">Disabled</span>
        </div>
        
        <div class="switch-container">
          <span class="pin-label">IO36:</span>
          <label class="switch">
            <input type="checkbox" id="enableSwitch5" onchange="toggleEnable(5)">
            <span class="slider-switch"></span>
          </label>
          <span id="enableStatus5" class="status-text">Disabled</span>
        </div>
      </div>
    </div>
    
    <div class="control-panel">
      <div class="direction">Clockwise ← → Counter-clockwise</div>
      <div class="slider-container">
        <input type="range" min="-100" max="100" value="0" class="slider" id="speedSlider">
      </div>
      <div class="speed-display">
        Speed: <span id="speedValue">0</span>%
      </div>
      <div class="direction" id="directionText">Stop</div>
    </div>
    
    <div class="button-container">
      <button class="button stop-btn" onclick="setSpeed(0)">Stop</button>
      <button class="button preset-btn" onclick="setSpeed(-50)">Half CW</button>
      <button class="button preset-btn" onclick="setSpeed(50)">Half CCW</button>
      <button class="button preset-btn" onclick="setSpeed(-100)">Full CW</button>
      <button class="button preset-btn" onclick="setSpeed(100)">Full CCW</button>
    </div>
    
    <div class="status">Status: <span id="connectionStatus">Connected</span></div>
  </div>
  <script>
    var slider = document.getElementById("speedSlider");
    var speedValue = document.getElementById("speedValue");
    var directionText = document.getElementById("directionText");
    var connectionStatus = document.getElementById("connectionStatus");
    
    // 更新滑块显示
    slider.oninput = function() {
      updateSpeedDisplay(this.value);
      setSpeed(this.value);
    }
    
    // 更新速度显示
    function updateSpeedDisplay(value) {
      speedValue.innerHTML = Math.abs(value);
      
      if (value == 0) {
        directionText.innerHTML = "Stop";
      } else if (value < 0) {
        directionText.innerHTML = "Clockwise";
      } else {
        directionText.innerHTML = "Counter-clockwise";
      }
    }
    
    // 设置速度
    function setSpeed(speed) {
      // 平滑过渡：避免直接从正值跳到负值或从负值跳到正值
      var currentSpeed = parseInt(slider.value);
      
      // 如果是从正转变为负转（或相反），先经过0
      if ((currentSpeed > 0 && speed < 0) || (currentSpeed < 0 && speed > 0)) {
        // 先设置为0，然后延迟300ms再设置为目标速度
        sendSpeedRequest(0);
        slider.value = 0;
        updateSpeedDisplay(0);
        
        setTimeout(function() {
          slider.value = speed;
          updateSpeedDisplay(speed);
          sendSpeedRequest(speed);
        }, 300);
      } else {
        // 同向变速，直接设置
        slider.value = speed;
        updateSpeedDisplay(speed);
        sendSpeedRequest(speed);
      }
    }
    
    // 发送速度请求到服务器
    function sendSpeedRequest(speed) {
      fetch('/setSpeed?value=' + speed)
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response was not ok');
          }
          connectionStatus.innerHTML = "Connected";
          return response.text();
        })
        .catch(error => {
          connectionStatus.innerHTML = "Connection Error";
          console.error('There was a problem with the fetch operation:', error);
        });
    }
    
    // 切换指定GPIO引脚的使能状态
    function toggleEnable(pinIndex) {
      var enableSwitch = document.getElementById("enableSwitch" + pinIndex);
      var enableStatus = document.getElementById("enableStatus" + pinIndex);
      
      fetch('/setEnable?pin=' + pinIndex + '&state=' + (enableSwitch.checked ? '1' : '0'))
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response was not ok');
          }
          enableStatus.innerHTML = enableSwitch.checked ? "Enabled" : "Disabled";
          connectionStatus.innerHTML = "Connected";
          return response.text();
        })
        .catch(error => {
          connectionStatus.innerHTML = "Connection Error";
          console.error('There was a problem with the fetch operation:', error);
        });
    }
    
    // 页面加载时获取当前状态
    window.onload = function() {
      // 可以添加获取当前使能状态的代码
    }
  </script>
</body>
</html>
  )rawliteral";
  
  server.send(200, "text/html; charset=UTF-8", html);
}
// 处理设置速度请求
void handleSetSpeed() {
  if (server.hasArg("value")) {
    String valueStr = server.arg("value");
    currentSpeed = valueStr.toInt();
    
    // 将速度值转换为舵机PWM值
    int pwmValue = mapSpeed(currentSpeed);
    
    // 设置舵机速度
    pwm_0.setPWM(servoChannel, 0, pwmValue);
    
    server.send(200, "text/plain", "Speed set to: " + valueStr);
    
    Serial.print("Speed: ");
    Serial.print(currentSpeed);
    Serial.print("%, PWM value: ");
    Serial.println(pwmValue);
  } else {
    server.send(400, "text/plain", "Missing value parameter");
  }
}
// 处理设置引脚使能状态请求
void handleSetEnable() {
  if (server.hasArg("pin") && server.hasArg("state")) {
    int pinIndex = server.arg("pin").toInt();
    String stateStr = server.arg("state");
    
    // 验证pinIndex是否在有效范围内
    if (pinIndex >= 0 && pinIndex < 6) {
      enableState[pinIndex] = (stateStr == "1");
      
      digitalWrite(enablePin[pinIndex], enableState[pinIndex] ? HIGH : LOW);
      
      server.send(200, "text/plain", String(pinNames[pinIndex]) + " set to: " + (enableState[pinIndex] ? "ON" : "OFF"));
      
      Serial.print(pinNames[pinIndex]);
      Serial.print(" Enable state: ");
      Serial.println(enableState[pinIndex] ? "ON" : "OFF");
    } else {
      server.send(400, "text/plain", "Invalid pin index");
    }
  } else {
    server.send(400, "text/plain", "Missing pin or state parameter");
  }
}