
将 CrossPoint 移植到 T-Deck-Max ；
注意不用兼容之前的板子，是单独为 T-Deck-Max 适配；


|       参数       |      T-Deck-MAX          |
| :--------------: | :----------------------------: |
|       MCU        |            ESP32-S3            |
|  Flash / PSRAM   |            16M / 8M            |
|       LoRa       |             SX1262             |
|       GPS        |            MIA-M10Q            |
|     显示屏       |      GDEQ031T10 (240x320)      |
|    4G 模块      |             A7682E             |
| 电池容量 |          3.7V-1500mAh          |
|   电池芯片   | SY6970 (0x6A), BQ27220 (0x55) |
|      音频       |         ES8311 (0x18)          |
|      触摸       |         CST328 (0x1A)          |
|    陀螺仪     |        BHI260AP (0x28)         |
|     键盘     |         TCA8418 (0x34)         |
|   IO 扩展   |         XL9555 (0x20)          |
|      马达       |         DRV2605 (0x5A)         |

PlatformIO 基础配置
platform = espressif32@6.13.0
board = T-Deck-Max
framework = arduino
monitor_speed = 115200
upload_speed = 921600

屏幕驱动使用 zinggjm/GxEPD2@^1.6.9 库对应的 GxEPD2_310_GDEQ031T10 型号；不要使用 open-x4-sdk

扩展芯片 XL9555 使用驱动 lewisxhe/SensorLib@^0.4.1；

触摸驱动使用 lib/HynTouch 

SY6970 使用 lewisxhe/XPowersLib@^0.3.3

键盘使用 adafruit/Adafruit TCA8418@^1.0.2
键盘布局
~~~cpp
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 10
#define KEYPAD_KEY_NONE  '\0'
#define KEYPAD_KEY_DEL   '\b'
#define KEYPAD_KEY_SPACE ' '
#define KEYPAD_KEY_ALT   '2'
#define KEYPAD_KEY_ENT   'E'
#define KEYPAD_KEY_UP    'U'
#define KEYPAD_KEY_SYM   'S'

Adafruit_TCA8418 keypad;

const char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', KEYPAD_KEY_DEL},
    {KEYPAD_KEY_ALT, 'z', 'x', 'c', 'v', 'b', 'n', 'm', '$', KEYPAD_KEY_ENT},
    {KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_NONE, KEYPAD_KEY_UP, '0', KEYPAD_KEY_SPACE, KEYPAD_KEY_SYM, KEYPAD_KEY_UP},
};
~~~

CrossPoint 按键映射
| CrossPoint |   T-Deck-Max   |
| :--------: | :------------: |
|    Back    | KEYPAD_KEY_DEL |
|  Confirm   | KEYPAD_KEY_ENT |
|    Left    |       A        |
|   Right    |       D        |
|     Up     |       W        |
|    Down    |       S        |
|   Power    |      BOOT      |


引脚映射：[pinmap](./pinmap_cn.md)


CrossPoint 项目路径：D:\dgx\code\2_studycode\crosspoint-reader
T-Deck-Max 项目路径：D:\dgx\code\0_lilygo\T_Deck_Pro_MAX
