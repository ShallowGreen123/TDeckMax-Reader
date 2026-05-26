
## :one: 产品规格 🎁

|       参数       |      T-Deck-MAX          |
| :--------------: | :----------------------------: |
|       MCU        |            ESP32-S3            |
|  Flash / PSRAM   |            16M / 8M            |
|       LoRa       |             SX1262             |
|       GPS        |            MIA-M10Q            |
|     显示屏       |      GDEQ031T10 (320x240)      |
|    4G 模块      |             A7682E             |
| 电池容量 |          3.7V-1500mAh          |
|   电池芯片   | SY6970 (0x6A), BQ27220 (0x55) |
|      音频       |         ES8311 (0x18)          |
|      触摸       |         CST328 (0x1A)          |
|    陀螺仪     |        BHI260AP (0x28)         |
|     键盘     |         TCA8418 (0x34)         |
|   IO 扩展   |         XL9555 (0x20)          |
|      马达       |         DRV2605 (0x5A)         |


屏幕驱动使用 zinggjm/GxEPD2@^1.6.9 库对应的 GxEPD2_310_GDEQ031T10 型号；
扩展芯片 XL9555 使用驱动 lewisxhe/SensorLib@^0.4.1；
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
|    Down    |       A        |
|   Power    |      BOOT      |


### 2. ES8311

❗ 注意：A7682E 和 ES8311 的扬声器是共用的。将 `XL9555` 的 `IO12` 设为 `LOW` 可输出 ES8311 音频。
声音太小时：将 `XL9555` 的 `IO06` 设为 `HIGH` 启用功率放大器。

### 3. LoRa

注意：
 使用内置天线时，将 `XL9555` 的 `IO04` 设为 `HIGH`。<br><br>使用外置天线时，将 `XL9555` 的 `IO04` 设为 `LOW`。<br><br>当前仓库以 `HIGH` 表示内置天线，`LOW` 表示外置天线。 

