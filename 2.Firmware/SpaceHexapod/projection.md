pwm_0 (0x40) 

pwm_1 (0x41)

**servo15-17 左前腿**

servo15 -> Axis01 -> pwm_1.setPWM(0,x,x) 

servo16 -> Axis11 -> pwm_1.setPWM(1,x,x) 

servo17-> Axis21 -> pwm_1.setPWM(2,x,x) 

**servo12-14 左中腿**

servo12 -> Axis02 -> pwm_0.setPWM(4,x,x) 

servo13 -> Axis12 -> pwm_0.setPWM(5,x,x)

servo14 -> Axis22 -> pwm_0.setPWM(6,x,x)

**servo9-11 左后腿**

servo9 -> Axis03 -> pwm_0.setPWM(1,x,x) 

servo10 -> Axis13 -> pwm_0.setPWM(2,x,x)

servo11-> Axis23 -> pwm_0.setPWM(3,x,x) 

**servo6-8 右前腿**

servo6 -> Axis06 -> pwm_0.setPWM(9,x,x) 

servo7 -> Axis16 -> pwm_0.setPWM(8,x,x) 

servo8 -> Axis26 -> pwm_0.setPWM(0,x,x) 

**servo3-5 右中腿**

servo3 -> Axis05 -> pwm_0.setPWM(12,x,x)

servo4 -> Axis15 -> pwm_0.setPWM(11,x,x)

servo5 -> Axis25 -> pwm_0.setPWM(10,x,x) 

**servo0-2 右后腿**

servo0 -> Axis04 -> pwm_0.setPWM(15,x,x)

servo1 -> Axis14 -> pwm_0.setPWM(14,x,x) 

servo2 -> Axis24 -> pwm_0.setPWM(13,x,x)

**servo18-19 气瓶**

servo18 -> pwm_1.setPWM(14,x,x) 

servo19 -> pwm_1.setPWM(3,x,x)

**IO 电磁铁**

1 -> IO19

2 -> IO18

<u>3 -> IO5</u>

4 -> IO27

5 -> IO14

<u>6 -> IO12</u>