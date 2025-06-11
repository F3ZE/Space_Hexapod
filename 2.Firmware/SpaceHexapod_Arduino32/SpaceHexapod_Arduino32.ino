/*******************************************************
   主板：SpaceHexapod_esp32_controlUnit
   功能：空间六足机器人Arduino程序esp32主控
   引脚：SDA:21   SCL:22
   Designer: Allen
   regenerator:SF
   Date:2025-4-17
*******************************************************/
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "Stepdata.h"
#define led 0
#define MAX_SRV_CLIENTS 3   //最大同时联接数，即你想要接入的设备数量，8266tcpserver只能接入五个

const char *ssid = "Baize2"; 
const char *password = "baizerobot"; 
//修改上
// 定义电磁铁控制引脚（按实际接线修改）
const int emagPins[6] = {32, 35, 36, 39, 34, 33}; // 对应腿部1~6
const int pinRemapPwm1[15] = {4,5,6,1,2,3,15,14,13,12,11,10,9,8,0};
//修改下

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();               //驱动1~16或(0~15)号舵机
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x41);          //驱动17~32或(16~31)号舵机

WiFiServer server(8266);//你要的端口号，随意修改，范围0-65535
WiFiClient serverClients[MAX_SRV_CLIENTS];

//#define SERVOMIN  102               //0.5/20 * 4096 = 102
//#define SERVOMID  307               //1.5/20 * 4096 = 307
//#define SERVOMAX  512               //2.5/20 * 4096 = 512
//实际测试
#define SERVOMIN  102               
#define SERVOMID  327               
#define SERVOMAX  552

//pwm.setPWM(i, 0, pulselen);第一个参数是通道数;第二个是高电平起始点，也就是从0开始;第三个参数是高电平终止点。

char cmd = 'e';//a:forward;   b:backward;   c:left;   d:right;   e:stop;

int rec[18] = {317,297,317,
               317,337,317,
               317,357,317,
               317,297,290,
               377,337,317,
               317,337,317};
int direct[18] = {-1,-1,-1,
-1,-1,-1,
-1,-1,-1,
-1,-1,-1,
-1,-1,-1,
-1,1,-1
};

void setup() {
  //修改上
  // 在server.begin()前添加
  for(int i=0;i<6;i++){
    pinMode(emagPins[i], OUTPUT);
    digitalWrite(emagPins[i], LOW); // 初始关闭
  }
  //修改下
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  Serial.println("UnderWaterHexapodRobot program!");
  
  pwm.begin();
  pwm1.begin();
  pwm.setPWMFreq(50);  // Analog servos run at ~50 Hz updates
  pwm1.setPWMFreq(50);  // Analog servos run at ~50 Hz updates

    for(int i=0;i<15;i++)
    {
      int p = pinRemapPwm1[i];
      pwm.setPWM(p, 0, rec[i]);
      
    }
      pwm1.setPWM(0, 0, rec[15]);
      pwm1.setPWM(1, 0, rec[16]);
      pwm1.setPWM(2, 0, rec[17]);
  delay(1000);
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
        delay(500);
  }
  server.begin();
  server.setNoDelay(true);  //加上后才正常些

}

void loop() {
  

   uint8_t i;

    if (server.hasClient())//WIFISETUP
    {
        for (i = 0; i < MAX_SRV_CLIENTS; i++)
        {
            if (!serverClients[i] || !serverClients[i].connected())
            {
                if (serverClients[i]) serverClients[i].stop();//未联接,就释放
                serverClients[i] = server.available();//分配新的
                continue;
            }
 
        }
        WiFiClient serverClient = server.available();
        serverClient.stop();
    }
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (serverClients[i] && serverClients[i].connected())
        {
            digitalWrite(led, 0);//有链接存在,就一直长亮
 
            if (serverClients[i].available())
            {
                while (serverClients[i].available()) 
                cmd = serverClients[i].read();
                delay(1);
                Serial.write(serverClients[i].read());
            }
        }
    }

/*
      for(int j=0;j<120;j++)
      {
          for(int i=0;i<16;i++)
          {
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forwardF[j][p]*direct[p],-90,90,-225,225)+rec[p]);
            
          }
          pwm1.setPWM(0, 0, map(forwardF[j][15]*direct[15],-90,90,-225,225)+rec[15]);
          pwm1.setPWM(1, 0, map(forwardF[j][16]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(2, 0, map(forwardF[j][17]*direct[17],-90,90,-225,225)+rec[17]);

          delay(10);
      }
*/

    if(cmd == 'a')//前进
    {
          for(int j=0;j<40;j++)
          {
        //       //-- 控制电磁铁：将120个周期分为10个条件，每个条件控制不同腿 --
        //   for (int leg = 0; leg < 6; leg++) {
        //   int condition = j / 12; // 将j=0~119分为10个条件（0~9），每个条件占12个j值
        //   // 根据条件选择需要吸附的腿（预留逻辑，用户需按需填充）
        //   switch (condition) {
        //     case 0: // 条件0（j=0~11）：控制腿1和4（索引0和3）
        //         digitalWrite(emagPins[0], LOW); // 腿1
        //         digitalWrite(emagPins[1], HIGH);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], HIGH);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], HIGH);  // 腿6
        //       break;
        //     case 1: // 条件1（j=12~23）：控制腿2和5（索引1和4）
        //         digitalWrite(emagPins[0], LOW); // 腿1
        //         digitalWrite(emagPins[1], HIGH);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], HIGH);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], HIGH);  // 腿6
        //       break;
        //     case 2: // 条件2（j=24~35）：控制腿3和6（索引2和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], HIGH);  // 腿2
        //         digitalWrite(emagPins[2], LOW); // 腿3
        //         digitalWrite(emagPins[3], HIGH);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], HIGH);  // 腿6
        //       break;
        //     case 3: // 条件3（j=36~47）：控制腿1和2（索引0和1）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], HIGH);  // 腿2
        //         digitalWrite(emagPins[2], LOW); // 腿3
        //         digitalWrite(emagPins[3], HIGH);  // 腿4
        //         digitalWrite(emagPins[4], LOW); // 腿5
        //         digitalWrite(emagPins[5], HIGH);  // 腿6
        //       break;
        //     case 4: // 条件4（j=48~59）：控制腿3和4（索引2和3）digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[0], HIGH);  // 腿1
        //         digitalWrite(emagPins[1], HIGH);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], HIGH);  // 腿4
        //         digitalWrite(emagPins[4], LOW); // 腿5
        //         digitalWrite(emagPins[5], HIGH);  // 腿6digitalWrite(emagPins[leg], (leg == 1 || leg == 2|| leg == 3|| leg == 4|| leg == 5|| leg == 0) ? HIGH : LOW : LOW : LOW : LOW : LOW);
        //       break;
        //     case 5: // 条件5（j=60~71）：控制腿5和6（索引4和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], HIGH);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], HIGH);  // 腿6
        //       break;
        //     case 6: // 条件6（j=72~83）：控制腿1和5（索引0和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], HIGH);  // 腿6
        //       break;
        //     case 7: // 条件7（j=84~95）：控制腿2和6（索引1和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], HIGH);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], HIGH);  // 腿6
        //       break;
        //     case 8: // 条件8（j=96~107）：控制腿3和5（索引2和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], HIGH);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 9: // 条件9（j=108~119）：控制腿4和6（索引3和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], HIGH);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], HIGH);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     default: // 超出范围时关闭所有腿
        //       digitalWrite(emagPins[leg], LOW);
        //   }
        // }

        //-- 原有舵机控制代码 --
          for(int i=0;i<15;i++)
          {
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forward[j][i]*direct[i],-90,90,-225,225)+rec[i]);
            
          }

          pwm1.setPWM(0, 0, map(forward[j][15]*direct[15],-90,90,-225,225)+rec[15]);
          pwm1.setPWM(1, 0, map(forward[j][16]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(2, 0, map(forward[j][17]*direct[17],-90,90,-225,225)+rec[17]);

          delay(10);
      }
    }


    else if(cmd == 'b')//后退
    {
      for(int j=0;j<40;j++)
      {
        //    //-- 控制电磁铁：将120个周期分为10个条件，每个条件控制不同腿 --
        //   for (int leg = 0; leg < 6; leg++) {
        //   int condition = j / 12; // 将j=0~119分为10个条件（0~9），每个条件占12个j值
        //   // 根据条件选择需要吸附的腿（预留逻辑，用户需按需填充）
        //   switch (condition) {
        //     case 0: // 条件0（j=0~11）：控制腿1和4（索引0和3）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 1: // 条件1（j=12~23）：控制腿2和5（索引1和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 2: // 条件2（j=24~35）：控制腿3和6（索引2和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 3: // 条件3（j=36~47）：控制腿1和2（索引0和1）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 4: // 条件4（j=48~59）：控制腿3和4（索引2和3）digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6digitalWrite(emagPins[leg], (leg == 1 || leg == 2|| leg == 3|| leg == 4|| leg == 5|| leg == 0) ? HIGH : LOW : LOW : LOW : LOW : LOW);
        //       break;
        //     case 5: // 条件5（j=60~71）：控制腿5和6（索引4和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 6: // 条件6（j=72~83）：控制腿1和5（索引0和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 7: // 条件7（j=84~95）：控制腿2和6（索引1和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 8: // 条件8（j=96~107）：控制腿3和5（索引2和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 9: // 条件9（j=108~119）：控制腿4和6（索引3和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     default: // 超出范围时关闭所有腿
        //       digitalWrite(emagPins[leg], LOW);
        //   }
        // }
          // 原有舵机控制代码
          for(int i=0;i<15;i++)
          {
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forward[39-j][i]*direct[i],-90,90,-225,225)+rec[i]);
            
          }
          pwm1.setPWM(0, 0, map(forward[39-j][15]*direct[15],-90,90,-225,225)+rec[15]);
          pwm1.setPWM(1, 0, map(forward[39-j][16]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(2, 0, map(forward[39-j][17]*direct[17],-90,90,-225,225)+rec[17]);
          delay(10);
      }      
      
    }
    else if(cmd == 'c')
    {
      for(int j=0;j<40;j++)
      {
        //  //-- 控制电磁铁：将120个周期分为10个条件，每个条件控制不同腿 --
        //   for (int leg = 0; leg < 6; leg++) {
        //   int condition = j / 12; // 将j=0~119分为10个条件（0~9），每个条件占12个j值
        //   // 根据条件选择需要吸附的腿（预留逻辑，用户需按需填充）
        //   switch (condition) {
        //     case 0: // 条件0（j=0~11）：控制腿1和4（索引0和3）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 1: // 条件1（j=12~23）：控制腿2和5（索引1和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 2: // 条件2（j=24~35）：控制腿3和6（索引2和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 3: // 条件3（j=36~47）：控制腿1和2（索引0和1）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 4: // 条件4（j=48~59）：控制腿3和4（索引2和3）digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6digitalWrite(emagPins[leg], (leg == 1 || leg == 2|| leg == 3|| leg == 4|| leg == 5|| leg == 0) ? HIGH : LOW : LOW : LOW : LOW : LOW);
        //       break;
        //     case 5: // 条件5（j=60~71）：控制腿5和6（索引4和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 6: // 条件6（j=72~83）：控制腿1和5（索引0和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 7: // 条件7（j=84~95）：控制腿2和6（索引1和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 8: // 条件8（j=96~107）：控制腿3和5（索引2和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 9: // 条件9（j=108~119）：控制腿4和6（索引3和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     default: // 超出范围时关闭所有腿
        //       digitalWrite(emagPins[leg], LOW);
        //   }
        // }
          // 原有舵机控制代码
          for(int i=0;i<6;i++)
          { 
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forward[j][i]*direct[i],-90,90,-225,225)+rec[i]);
            
          }
          for(int i=6;i<15;i++)
          { 
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forward[39-j][i]*direct[i],-90,90,-225,225)+rec[i]);
            
          }
          pwm.setPWM(0, 0, map(forward[j][15]*direct[15],-90,90,-225,225)+rec[15]);
          pwm1.setPWM(1, 0, map(forward[j][16]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(2, 0, map(forward[j][17]*direct[17],-90,90,-225,225)+rec[17]);
          delay(10);
      }
    }
    else if(cmd == 'd')
    {
      for(int j=0;j<40;j++)
      {
        //  //-- 控制电磁铁：将120个周期分为10个条件，每个条件控制不同腿 --
        //   for (int leg = 0; leg < 6; leg++) {
        //   int condition = j / 12; // 将j=0~119分为10个条件（0~9），每个条件占12个j值
        //   // 根据条件选择需要吸附的腿（预留逻辑，用户需按需填充）
        //   switch (condition) {
        //     case 0: // 条件0（j=0~11）：控制腿1和4（索引0和3）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 1: // 条件1（j=12~23）：控制腿2和5（索引1和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 2: // 条件2（j=24~35）：控制腿3和6（索引2和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 3: // 条件3（j=36~47）：控制腿1和2（索引0和1）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 4: // 条件4（j=48~59）：控制腿3和4（索引2和3）digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6digitalWrite(emagPins[leg], (leg == 1 || leg == 2|| leg == 3|| leg == 4|| leg == 5|| leg == 0) ? HIGH : LOW : LOW : LOW : LOW : LOW);
        //       break;
        //     case 5: // 条件5（j=60~71）：控制腿5和6（索引4和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 6: // 条件6（j=72~83）：控制腿1和5（索引0和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 7: // 条件7（j=84~95）：控制腿2和6（索引1和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 8: // 条件8（j=96~107）：控制腿3和5（索引2和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 9: // 条件9（j=108~119）：控制腿4和6（索引3和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     default: // 超出范围时关闭所有腿
        //       digitalWrite(emagPins[leg], LOW);
        //   }
        // }
          // 原有舵机控制代码
          for(int i=0;i<6;i++)
          {
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forward[119-j][i]*direct[i],-90,90,-225,225)+rec[i]);          
          }
          for(int i=6;i<15;i++)
          {
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forward[j][i]*direct[i],-90,90,-225,225)+rec[i]);            
          }
          pwm.setPWM(0, 0, map(forward[39-j][15]*direct[15],-90,90,-225,225)+rec[15]);
          pwm1.setPWM(1, 0, map(forward[39-j][16]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(2, 0, map(forward[39-j][17]*direct[17],-90,90,-225,225)+rec[17]);
          delay(10);
      }
    }
    else if(cmd == 'f')//右横移前进
    {
      for(int j=0;j<120;j++)
      {
        //  //-- 控制电磁铁：将120个周期分为10个条件，每个条件控制不同腿 --
        //   for (int leg = 0; leg < 6; leg++) {
        //   int condition = j / 12; // 将j=0~119分为10个条件（0~9），每个条件占12个j值
        //   // 根据条件选择需要吸附的腿（预留逻辑，用户需按需填充）
        //   switch (condition) {
        //     case 0: // 条件0（j=0~11）：控制腿1和4（索引0和3）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 1: // 条件1（j=12~23）：控制腿2和5（索引1和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 2: // 条件2（j=24~35）：控制腿3和6（索引2和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 3: // 条件3（j=36~47）：控制腿1和2（索引0和1）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 4: // 条件4（j=48~59）：控制腿3和4（索引2和3）digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 5: // 条件5（j=60~71）：控制腿5和6（索引4和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 6: // 条件6（j=72~83）：控制腿1和5（索引0和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 7: // 条件7（j=84~95）：控制腿2和6（索引1和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 8: // 条件8（j=96~107）：控制腿3和5（索引2和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 9: // 条件9（j=108~119）：控制腿4和6（索引3和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     default: // 超出范围时关闭所有腿
        //       digitalWrite(emagPins[leg], LOW);
        //   }
        // }
          // 原有舵机控制代码
          for(int i=0;i<15;i++)
          {
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forwardFHer[j][i]*direct[i],-90,90,-225,225)+rec[i]);            
          }
          pwm1.setPWM(0, 0, map(forwardFHer[j][15]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(1, 0, map(forwardFHer[j][16]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(2, 0, map(forwardFHer[j][17]*direct[17],-90,90,-225,225)+rec[17]);
          delay(10);
      }
    }
    else if(cmd == 'g')//左横移前进
    {
      for(int j=0;j<120;j++)
      {
        //  //-- 控制电磁铁：将120个周期分为10个条件，每个条件控制不同腿 --
        //   for (int leg = 0; leg < 6; leg++) {
        //   int condition = j / 12; // 将j=0~119分为10个条件（0~9），每个条件占12个j值
        //   // 根据条件选择需要吸附的腿（预留逻辑，用户需按需填充）
        //   switch (condition) {
        //     case 0: // 条件0（j=0~11）：控制腿1和4（索引0和3）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 1: // 条件1（j=12~23）：控制腿2和5（索引1和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 2: // 条件2（j=24~35）：控制腿3和6（索引2和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 3: // 条件3（j=36~47）：控制腿1和2（索引0和1）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 4: // 条件4（j=48~59）：控制腿3和4（索引2和3）digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6digitalWrite(emagPins[leg], (leg == 1 || leg == 2|| leg == 3|| leg == 4|| leg == 5|| leg == 0) ? HIGH : LOW : LOW : LOW : LOW : LOW);
        //       break;
        //     case 5: // 条件5（j=60~71）：控制腿5和6（索引4和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 6: // 条件6（j=72~83）：控制腿1和5（索引0和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 7: // 条件7（j=84~95）：控制腿2和6（索引1和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 8: // 条件8（j=96~107）：控制腿3和5（索引2和4）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     case 9: // 条件9（j=108~119）：控制腿4和6（索引3和5）
        //         digitalWrite(emagPins[0], HIGH); // 腿1
        //         digitalWrite(emagPins[1], LOW);  // 腿2
        //         digitalWrite(emagPins[2], HIGH); // 腿3
        //         digitalWrite(emagPins[3], LOW);  // 腿4
        //         digitalWrite(emagPins[4], HIGH); // 腿5
        //         digitalWrite(emagPins[5], LOW);  // 腿6
        //       break;
        //     default: // 超出范围时关闭所有腿
        //       digitalWrite(emagPins[leg], LOW);
        //   }
        // }
          // 原有舵机控制代码
          for(int i=0;i<15;i++)
          {
            int p = pinRemapPwm1[i];
            pwm.setPWM(p, 0, map(forwardFHer[119-j][i]*direct[i],-90,90,-225,225)+rec[i]);
            
          }
          pwm1.setPWM(0, 0, map(forwardFHer[119-j][15]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(1, 0, map(forwardFHer[119-j][16]*direct[16],-90,90,-225,225)+rec[16]);
          pwm1.setPWM(2, 0, map(forwardFHer[119-j][17]*direct[17],-90,90,-225,225)+rec[17]);
          delay(10);
      }      
    }
    else
    {

      //pwm.setPWM(6, 0, 0+rec[3]);
      //     // 关闭所有电磁铁
      // for (int leg = 0; leg < 6; leg++) {
      //   digitalWrite(emagPins[leg], LOW);
      // }
      delay(100);
    }
}

void blink()
{
    static long previousMillis = 0;
    static int currstate = 0;
 
    if (millis() - previousMillis > 200)  //200ms
    {
        previousMillis = millis();
        currstate = 1 - currstate;
        digitalWrite(led, currstate);
    }
}
