#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "driver/i2c.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "main.h"
#include "math.h"

// 日志标签
static const char* TAG = "HEXAPOD_CONTROL";

// 定义所有舵机轴
servo_axis_t servo_axes[NUM_SERVOS] = {
    {1, 0, "Axis01", 90},
    {1, 1, "Axis11", 90},
    {1, 2, "Axis21", 90},
    {0, 4, "Axis02", 90},
    {0, 5, "Axis12", 90},
    {0, 6, "Axis22", 90},
    {0, 1, "Axis03", 90},
    {0, 2, "Axis13", 90},
    {0, 3, "Axis23", 90},
    {0, 9, "Axis06", 90},
    {0, 8, "Axis16", 90},
    {0, 0, "Axis26", 90},
    {0, 12, "Axis05", 90},
    {0, 11, "Axis15", 90},
    {0, 10, "Axis25", 90},
    {0, 15, "Axis04", 90},
    {0, 14, "Axis14", 90},
    {0, 13, "Axis24", 90},
    {1, 14, "Servo18", 90},
    {1, 3, "Servo19", 90}
};

// 步态控制相关变量
#define SERVO_COUNT 18   // 实际控制的舵机数量(不含额外的两个舵机)
float utotal[SERVO_COUNT]; // 舵机角度数组
float uf1 = 0, uf2 = 0, uf1_double = 0, uf2_double = 0; // 波函数值
int tp = 0, T1 = 20; // 步态周期参数
float pi = 3.1415926;
bool is_walking = false; // 是否在行走
int direction = 0; // 行走方向: -1后退, 0停止, 1前进

// 步态参数
float Hb1 = 20; // 大腿弯曲角
float Hb2 = -5; // 小腿弯曲角
float Hs0 = 35; // 髋关节摆动比例
float Hs1 = 25; // 大腿摆动比例
float Hv2 = 5.5; // 小腿摆动加权
float Ho = 55; // 髋关节摆动偏移

// 基础角度值（中心位置）
int base_angles[SERVO_COUNT] = {
  30, 45, 120,// 左前腿 (髋、大腿、小腿)
  90, 45, 120, // 左中腿
  150, 45, 120, // 左后腿
  25, 125, 60, // 右后腿
  80, 125, 60, // 右中腿
  155, 125, 60 // 右前腿
};

int k[SERVO_COUNT] = {
  -1, -1, -1, // 左前腿 (髋、大腿、小腿)
  -1, -1, -1, // 左中腿
  -1, -1, -1, // 左后腿
  -1, -1, -1, // 右后腿
  -1, -1, -1, // 右中腿
  -1, -1, -1 // 右前腿
};

// I2C初始化
static esp_err_t i2c_master_init(void) {
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = I2C_MASTER_FREQ_HZ,
  };
  esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
  if (err != ESP_OK) {
    return err;
  }
  return i2c_driver_install(I2C_NUM_0, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// 写入PCA9685寄存器
static esp_err_t pca9685_write_register(uint8_t addr, uint8_t reg, uint8_t data) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);
  i2c_master_write_byte(cmd, data, true);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

// 初始化PCA9685
static esp_err_t pca9685_init(uint8_t addr) {
  // 软件复位
  esp_err_t ret = pca9685_write_register(addr, PCA9685_MODE1, 0x80);
  if (ret != ESP_OK) {
    return ret;
  }
  vTaskDelay(10 / portTICK_PERIOD_MS);
  // 设置PWM频率
  uint8_t prescale = (uint8_t)(25000000 / (4096 * PCA9685_SERVO_FREQ * 0.98) - 1);
  ret = pca9685_write_register(addr, PCA9685_MODE1, 0x10); // 进入睡眠模式
  if (ret != ESP_OK) {
    return ret;
  }
  ret = pca9685_write_register(addr, PCA9685_PRESCALE, prescale);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = pca9685_write_register(addr, PCA9685_MODE1, 0xA1); // 退出睡眠模式，启用自动增量
  if (ret != ESP_OK) {
    return ret;
  }
  vTaskDelay(5 / portTICK_PERIOD_MS);
  return ESP_OK;
}

// 设置PWM脉冲
static esp_err_t pca9685_set_pwm(uint8_t addr, uint8_t channel, uint16_t on, uint16_t off) {
  uint8_t reg = PCA9685_LED0_ON_L + (channel * 4);
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg, true);
  i2c_master_write_byte(cmd, on & 0xFF, true);
  i2c_master_write_byte(cmd, on >> 8, true);
  i2c_master_write_byte(cmd, off & 0xFF, true);
  i2c_master_write_byte(cmd, off >> 8, true);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

// 将角度值(0-180)映射到舵机PWM值
static int map_angle(int angle) {
  int adjusted_pwm;
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  // 将角度映射到脉冲宽度
  adjusted_pwm = (angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180) + SERVO_MIN_PULSE;
  return adjusted_pwm;
}

// 设置舵机角度
static esp_err_t set_servo_angle(int servo_index, int angle) {
  if (servo_index < 0 || servo_index >= NUM_SERVOS) {
    return ESP_ERR_INVALID_ARG;
  }
  // 限制角度范围
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  servo_axis_t* servo = &servo_axes[servo_index];
  servo->angle = angle;
  int pwm_value = map_angle(angle);
  uint8_t addr = (servo->pwm_controller == 0) ? PCA9685_ADDR_0 : PCA9685_ADDR_1;
  esp_err_t ret = pca9685_set_pwm(addr, servo->channel, 0, pwm_value);

  ESP_LOGI(TAG, "舵机: %s 角度: %d PWM: %d", servo->name, angle, pwm_value);

  return ret;
}

// 更新步态函数 - 实现三角步态的前进后退功能
void updateGait() {
  tp++;
  if (tp > 0 && tp <= T1) {
    uf1 = (-cos(2 * pi * tp / T1) + 1.0) / 2.0;
    uf1_double = (-cos(1 * pi * tp / T1) + 1.0) / 2.0;
    uf2 = 0;
    uf2_double = (cos(1 * pi * tp / T1) + 1.0) / 2.0;
  } else if (tp > T1 && tp <= 2 * T1) {
    int tp0 = tp - T1;
    uf1 = 0;
    uf1_double = (cos(1 * pi * tp0 / T1) + 1.0) / 2.0;
    uf2 = (-cos(2 * pi * tp0 / T1) + 1.0) / 2.0;
    uf2_double = (-cos(1 * pi * tp0 / T1) + 1.0) / 2.0;
  }
  if (tp >= 2 * T1) tp = 0;

  // 清零
  for (int i = 0; i < SERVO_COUNT; i++) utotal[i] = 0;

  // 根据行走方向调整角度
  int dir_multiplier = direction; // 方向乘数：1前进，-1后退

  // tripod1: (0),(3),(9) - 髋关节
  utotal[0] -= Hs0 * (uf1_double - 0.5) * 1.1 * dir_multiplier + Ho;
  utotal[6] -= Hs0 * (uf1_double - 0.5) * 1.1 * dir_multiplier - Ho;
  utotal[12] += Hs0 * (uf1_double - 0.5) * dir_multiplier;

  // tripod2: (6),(12),(15) - 髋关节
  utotal[3] -= Hs0 * (uf2_double - 0.5) * dir_multiplier;
  utotal[9] += Hs0 * (uf2_double - 0.5) * 1.1 * dir_multiplier - Ho;
  utotal[15] += Hs0 * (uf2_double - 0.5) * 1.1 * dir_multiplier + Ho;

  // tripod1: (1,2),(7,8),(13,14) - 大腿和小腿
  utotal[1] -= Hs1 * uf1 + Hb1;
  utotal[7] -= Hs1 * uf1 + Hb1;
  utotal[13] += Hs1 * uf1 + Hb1;

  utotal[2] -= (Hs1 * uf1 + Hb2) * 1.1 * Hv2;
  utotal[8] -= (Hs1 * uf1 + Hb2) * 1.1 * Hv2;
  utotal[14] += (Hs1 * uf1 + Hb2) * Hv2;

  // tripod2: (4,5),(10,11),(16,17) - 大腿和小腿
  utotal[4] -= Hs1 * uf2 + Hb1;
  utotal[10] += Hs1 * uf2 + Hb1;
  utotal[16] += Hs1 * uf2 + Hb1;

  utotal[5] -= (Hs1 * uf2 + Hb2) * Hv2;
  utotal[11] += (Hs1 * uf2 + Hb2) * 1.1 * Hv2;
  utotal[17] += (Hs1 * uf2 + Hb2) * 1.1 * Hv2;

  for (int i = 0; i<3; i++) {
    if (utotal[i*3+2]<0)
      utotal[i*3+2] = 0;
    if (utotal[(i+3)*3+2]>0)
      utotal[(i+3)*3+2] = 0;
  }
}

// 将计算好的角度应用到舵机
void applyServoAngles() {
  // 将0计算的角度应用到实际舵机
  for (int i = 0; i < SERVO_COUNT; i++) {
    // 将计算结果加上基础角度
    int angle = (int)(base_angles[i]+utotal[i]*k[i]);
    // 确保角度在有效范围内
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    // 设置舵机角度
    set_servo_angle(i, angle);
  }
}

// 步态控制任务
void gaitControlTask(void* pvParameters) {
  TickType_t last_wake_time = xTaskGetTickCount();
  const TickType_t delay_time = 50 / portTICK_PERIOD_MS; // 20Hz更新频率

  while (1) {
    if (is_walking) {
      // 更新步态计算
      updateGait();
      // 应用到舵机
      applyServoAngles();
    }
    // 固定频率执行
    vTaskDelayUntil(&last_wake_time, delay_time);
  }
}

// 初始化站立姿势 - 让机器人进入标准姿势
void initStandingPose() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    // set_servo_angle(i, base_angles[i]);
    set_servo_angle(i, 90);
  }
  ESP_LOGI(TAG, "机器人初始化为站立姿势");
}

// 初始化步态控制
void initGaitControl() {
  // 创建步态控制任务
  xTaskCreate(gaitControlTask, "gait_control_task", 4096, NULL, 5, NULL);
}

// 设置行走状态和方向
void setWalkingState(bool walking, int walk_direction) {
  is_walking = walking;
  direction = walk_direction;
  ESP_LOGI(TAG, "设置行走状态: %s, 方向: %d", walking ? "行走" : "停止", direction);
}

// HTTP网页内容 - 简化版控制界面
static const char* html_page = "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "    <title>Hexapod Control</title>\n"
    "    <style>\n"
    "        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Arial', sans-serif; }\n"
    "        body { background-color: #f5f5f5; color: #333; padding: 20px; max-width: 600px; margin: 0 auto; }\n"
    "        .header { text-align: center; margin-bottom: 20px; padding: 15px; background: linear-gradient(135deg, #3498db, #2980b9); color: white; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }\n"
    "        .control-panel { display: flex; flex-direction: column; gap: 20px; margin-bottom: 30px; align-items: center; }\n"
    "        .buttons-row { display: flex; gap: 20px; justify-content: center; width: 100%; }\n"
    "        .action-button { padding: 20px 30px; font-size: 18px; border: none; border-radius: 8px; background-color: #3498db; color: white; cursor: pointer; transition: all 0.3s ease; width: 100%; text-align: center; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }\n"
    "        .action-button:hover { background-color: #2980b9; transform: translateY(-2px); }\n"
    "        .action-button:active { transform: translateY(1px); }\n"
    "        .stop-button { background-color: #e74c3c; }\n"
    "        .stop-button:hover { background-color: #c0392b; }\n"
    "        .status { padding: 15px; background-color: #fff; border-radius: 8px; margin-bottom: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); text-align: center; }\n"
    "        .status-indicator { display: inline-block; width: 15px; height: 15px; border-radius: 50%; background-color: #ccc; margin-right: 10px; }\n"
    "        .status-active { background-color: #2ecc71; }\n"
    "        @media (max-width: 480px) { .buttons-row { flex-direction: column; } }\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <div class=\"header\">\n"
    "        <h1>Hexapod Robot Control</h1>\n"
    "    </div>\n"
    "    \n"
    "    <div class=\"status\" id=\"status-panel\">\n"
    "        <h2>Status: <span id=\"status-text\">Stopped</span> <span class=\"status-indicator\" id=\"status-indicator\"></span></h2>\n"
    "    </div>\n"
    "    \n"
    "    <div class=\"control-panel\">\n"
    "        <button class=\"action-button\" id=\"forward-btn\" onclick=\"controlRobot('forward')\">Forward</button>\n"
    "        <div class=\"buttons-row\">\n"
    "            <button class=\"action-button stop-button\" id=\"stop-btn\" onclick=\"controlRobot('stop')\">STOP</button>\n"
    "        </div>\n"
    "        <button class=\"action-button\" id=\"backward-btn\" onclick=\"controlRobot('backward')\">Backward</button>\n"
    "    </div>\n"
    "\n"
    "    <script>\n"
    "        function updateStatus(status) {\n"
    "            const statusText = document.getElementById('status-text');\n"
    "            const statusIndicator = document.getElementById('status-indicator');\n"
    "            \n"
    "            statusText.textContent = status;\n"
    "            if (status === 'Stopped') {\n"
    "                statusIndicator.classList.remove('status-active');\n"
    "            } else {\n"
    "                statusIndicator.classList.add('status-active');\n"
    "            }\n"
    "        }\n"
    "\n"
    "        function controlRobot(command) {\n"
    "            fetch(`/control?cmd=${command}`)\n"
    "                .then(response => {\n"
    "                    if (!response.ok) {\n"
    "                        throw new Error('Network response error');\n"
    "                    }\n"
    "                    return response.text();\n"
    "                })\n"
    "                .then(data => {\n"
    "                    console.log('Command sent:', command);\n"
    "                    \n"
    "                    if (command === 'forward') {\n"
    "                        updateStatus('Moving Forward');\n"
    "                    } else if (command === 'backward') {\n"
    "                        updateStatus('Moving Backward');\n"
    "                    } else if (command === 'stop') {\n"
    "                        updateStatus('Stopped');\n"
    "                    }\n"
    "                })\n"
    "                .catch(error => {\n"
    "                    console.error('Error:', error);\n"
    "                    alert('Error sending command: ' + error.message);\n"
    "                });\n"
    "        }\n"
    "    </script>\n"
    "</body>\n"
    "</html>";

// HTTP请求处理函数 - 根路径
static esp_err_t root_get_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html_page, strlen(html_page));
  return ESP_OK;
}

// HTTP请求处理函数 - 控制机器人
static esp_err_t control_handler(httpd_req_t* req) {
  char* buf;
  size_t buf_len;
  char cmd[32] = {0};

  // 获取查询字符串长度
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      // 解析命令
      if (httpd_query_key_value(buf, "cmd", cmd, sizeof(cmd)) == ESP_OK) {
        ESP_LOGI(TAG, "接收到控制命令: %s", cmd);

        // 执行对应命令
        if (strcmp(cmd, "forward") == 0) {
          setWalkingState(true, 1); // 前进
        } else if (strcmp(cmd, "backward") == 0) {
          setWalkingState(true, -1); // 后退
        } else if (strcmp(cmd, "stop") == 0) {
          setWalkingState(false, 0); // 停止
        }
      }
    }
    free(buf);
  }

  // 返回成功响应
  httpd_resp_sendstr(req, "OK");
  return ESP_OK;
}


// 注册HTTP处理函数
httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL
};

httpd_uri_t control = {
    .uri = "/control",
    .method = HTTP_GET,
    .handler = control_handler,
    .user_ctx = NULL
};

// 配置并启动 HTTP 服务器
static httpd_handle_t start_webserver(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_handle_t server = NULL;
  if (httpd_start(&server, &config) == ESP_OK) {
    // 注册处理函数
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &control);
  }
  return server;
}

// 初始化 Wi-Fi 为 AP 模式
static void wifi_init_softap(void) {
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &(wifi_config_t){
                          .ap = {
                              .ssid = "Hexapod_Robot",
                              .password = "12345678",
                              .ssid_len = 0,
                              .max_connection = 4,
                              .authmode = WIFI_AUTH_WPA_WPA2_PSK
                          },
                      });

  esp_wifi_start();
  ESP_LOGI(TAG, "Wi-Fi AP 初始化成功, SSID: %s, 密码: %s", "Hexapod_Robot", "12345678");
}

void app_main(void) {
  // 初始化 NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(TAG, "NVS 初始化成功");

  // 初始化 I2C
  ESP_LOGI(TAG, "正在初始化 I2C...");
  ESP_ERROR_CHECK(i2c_master_init());
  ESP_LOGI(TAG, "I2C 初始化成功");

  // 初始化 PCA9685
  ESP_ERROR_CHECK(pca9685_init(PCA9685_ADDR_0));
  ESP_ERROR_CHECK(pca9685_init(PCA9685_ADDR_1));
  ESP_LOGI(TAG, "PCA9685 初始化成功");

  // 等待舵机到达初始位置
  vTaskDelay(500 / portTICK_PERIOD_MS);

  // 初始化站立姿势
  initStandingPose();

  // 等待姿势完成
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // 初始化步态控制
  initGaitControl();

  // 初始化 Wi-Fi AP
  wifi_init_softap();


  // set_servo_angle(0,0);
  // set_servo_angle(1,90);
  // set_servo_angle(2,180);
  // 启动 Web 服务器
  httpd_handle_t server = start_webserver();

  if (server) {
    ESP_LOGI(TAG, "Web 服务器启动成功，请连接到 Wi-Fi 并访问 http://192.168.4.1 控制机器人");
  } else {
    ESP_LOGE(TAG, "Web 服务器启动失败!");
  }

  // 主循环
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
