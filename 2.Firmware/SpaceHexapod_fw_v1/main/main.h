//
// Created by Enhao on 2025/6/15.
//

#ifndef MAIN_H
#define MAIN_H

#define I2C_MASTER_SCL_IO           22      // I2C SCL引脚
#define I2C_MASTER_SDA_IO           21      // I2C SDA引脚
#define I2C_MASTER_FREQ_HZ          400000  // I2C频率
#define I2C_MASTER_TX_BUF_DISABLE   0       // 不使用TX缓冲区
#define I2C_MASTER_RX_BUF_DISABLE   0       // 不使用RX缓冲区
#define I2C_MASTER_TIMEOUT_MS       1000
#define PCA9685_ADDR_0              0x40    // 第一个PCA9685地址
#define PCA9685_ADDR_1              0x41    // 第二个PCA9685地址
#define PCA9685_MODE1               0x00    // 模式寄存器1
#define PCA9685_PRESCALE            0xFE    // 预分频器寄存器
#define PCA9685_LED0_ON_L           0x06    // LED0起始寄存器
#define PCA9685_SERVO_FREQ          50      // 舵机PWM频率 (50Hz)
#define SERVO_MIN_PULSE             102.4     // 最小脉冲宽度
#define SERVO_MAX_PULSE             512.8     // 最大脉冲宽度
#define NUM_SERVOS                  20      // 舵机数量

// 舵机轴配置结构
typedef struct {
  uint8_t pwm_controller; // 0 for PCA9685_ADDR_0, 1 for PCA9685_ADDR_1
  uint8_t channel; // PWM通道号
  char name[32]; // 轴名称
  int angle; // 当前角度
} servo_axis_t;

static esp_err_t set_servo_angle(int servo_index, int angle);

#endif //MAIN_H
