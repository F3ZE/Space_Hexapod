#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>

// 网络设置
const char* ssid = "ServoControl";  // WiFi名称
const char* password = "12345678";  // WiFi密码

// 创建PWM控制器实例
Adafruit_PWMServoDriver pwm_0 = Adafruit_PWMServoDriver(0x40);  
Adafruit_PWMServoDriver pwm_1 = Adafruit_PWMServoDriver(0x41);

// 舵机PWM参数
#define SERVO_MIN_PULSE  102.4  // 最小脉冲宽度
#define SERVO_MAX_PULSE  512.8  // 最大脉冲宽度
#define SERVO_FREQ       50     // 舵机PWM频率 (50Hz)

// 舵机轴配置结构
struct ServoAxis {
  byte pwmController;  // 0 for pwm_0, 1 for pwm_1
  byte channel;        // PWM通道号
  String name;         // 轴名称
  int angle;           // 当前角度
};

// 定义所有舵机轴
ServoAxis servoAxes[] = {
  {1, 0, "Axis01 (servo15)", 90},
  {1, 1, "Axis11 (servo16)", 90},
  {1, 2, "Axis21 (servo17)", 90},
  {0, 4, "Axis02 (servo12)", 90},
  {0, 5, "Axis12 (servo13)", 90},
  {0, 6, "Axis22 (servo14)", 90},
  {0, 1, "Axis03 (servo9)", 90},
  {0, 2, "Axis13 (servo10)", 90},
  {0, 3, "Axis23 (servo11)", 90},
  {0, 9, "Axis06 (servo6)", 90},
  {0, 8, "Axis16 (servo7)", 90},
  {0, 0, "Axis26 (servo8)", 90},
  {0, 12, "Axis05 (servo3)", 90},
  {0, 11, "Axis15 (servo4)", 90},
  {0, 10, "Axis25 (servo5)", 90},
  {0, 15, "Axis04 (servo0)", 90},
  {0, 14, "Axis14 (servo1)", 90},
  {0, 13, "Axis24 (servo2)", 90},
  {1, 14, "Servo18", 90},
  {1, 3, "Servo19", 90}
};

// 计算总的舵机数量
const int TOTAL_SERVOS = sizeof(servoAxes) / sizeof(ServoAxis);

// 角度转PWM值函数
int angleToPWM(int angle) {
  // 限制角度范围在0-180度
  angle = constrain(angle, 0, 180);
  
  // 计算对应的PWM值
  int pwmValue = SERVO_MIN_PULSE + (angle / 180.0) * (SERVO_MAX_PULSE - SERVO_MIN_PULSE);
  return pwmValue;
}

// 设置单个舵机角度
void setServoAngle(int servoIndex, int angle) {
  if (servoIndex < 0 || servoIndex >= TOTAL_SERVOS) {
    Serial.println("错误：舵机索引超出范围");
    return;
  }
  
  ServoAxis* servo = &servoAxes[servoIndex];
  int pwmValue = angleToPWM(angle);
  
  // 根据PWM控制器选择对应的实例
  if (servo->pwmController == 0) {
    pwm_0.setPWM(servo->channel, 0, pwmValue);
  } else if (servo->pwmController == 1) {
    pwm_1.setPWM(servo->channel, 0, pwmValue);
  }
  
  // 更新角度记录
  servo->angle = angle;
  
  Serial.println(servo->name + " 设置到 " + String(angle) + "度");
}

// 设置所有舵机到指定角度
void setAllServosToAngle(int angle) {
  Serial.println("开始设置所有舵机到 " + String(angle) + "度...");
  
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    setServoAngle(i, angle);
    delay(50);  // 给每个舵机一点时间响应
  }
  
  Serial.println("所有舵机设置完成！");
}

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial.println("舵机控制程序启动...");
  
  // 初始化PWM控制器
  if (!pwm_0.begin()) {
    Serial.println("错误：PWM控制器0初始化失败！");
    return;
  }
  
  if (!pwm_1.begin()) {
    Serial.println("错误：PWM控制器1初始化失败！");
    return;
  }
  
  // 设置PWM频率
  pwm_0.setPWMFreq(SERVO_FREQ);
  pwm_1.setPWMFreq(SERVO_FREQ);
  
  Serial.println("PWM控制器初始化成功");
  Serial.println("检测到 " + String(TOTAL_SERVOS) + " 个舵机");
  
  // 等待一秒让系统稳定
  delay(1000);
  
  // 设置所有舵机到90度
  setAllServosToAngle(90);
  
  Serial.println("程序初始化完成，所有舵机已设置到90度位置");
}

void loop() {
  // 主循环保持空闲
  // 如果需要，可以在这里添加其他功能
  delay(1000);
}
