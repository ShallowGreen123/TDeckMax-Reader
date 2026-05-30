# T-Deck Pro Max Pin Map (V0.1 / 2025-09-11)

`English` | [中文](./pinmap_cn.md)

> This document is organized from `hardware/T-Deck Pro Max V0.1 25-09-11/T-Deck Pro Max V0.1 25-09-11.pdf`, and cross-checked against the current repository `readme.md`, `readme_cn.md`, and `examples/factory/utilities.h`.

> This document is grouped by module. Some schematic net names are named from the peripheral point of view, such as `GPS_TX`, `7682_RXD`, and `I2S_DSDIN`; this document also lists the commonly used board-level macro names found in the repository.

## 1. MCU and Shared Buses

| Function | Current / Recommended Macro | GPIO | Source | Notes |
| --- | --- | --- | --- | --- |
| Main I2C SDA | `BOARD_I2C_SDA` | 13 | Schematic Page 1/4/7/8 | Shared by the whole board |
| Main I2C SCL | `BOARD_I2C_SCL` | 14 | Schematic Page 1/4/7/8 | Shared by the whole board |
| Main SPI SCK | `BOARD_SPI_SCK` | 36 | Schematic Page 3/4/6 | Shared by EPD, TF, and SX1262 |
| Main SPI MOSI | `BOARD_SPI_MOSI` | 33 | Schematic Page 3/4/6 | Shared by EPD, TF, and SX1262 |
| Main SPI MISO | `BOARD_SPI_MISO` | 47 | Schematic Page 3/6 | Shared by TF and SX1262 |
| BOOT | `BOARD_BOOT_PIN` | 0 | Schematic Page 2 | Boot button |

## 2. XL9555 Expanded IO

I2C address: `0x20`.

`BOARD_XL9555_INT` is currently fixed to `-1` because the XL9555 `INT` pin is not connected to any ESP32 pin.

| XL9555 | Current / Recommended Macro | Board Net | Direction | Source | Notes |
| --- | --- | --- | --- | --- | --- |
| P00 | `BOARD_XL9555_00_6609_EN` | `6609_EN` | Output | Schematic Page 1/7 | A7682E power enable |
| P01 | `BOARD_XL9555_01_LORA_EN` | `LORA_EN` | Output | Schematic Page 3/7 | SX1262 power enable |
| P02 | `BOARD_XL9555_02_GPS_EN` | `GPS_EN` | Output | Schematic Page 3/7 | MIA-M10Q power enable |
| P03 | `BOARD_XL9555_03_1V8_EN` | `1V8_EN` | Output | Schematic Page 4/7 | BHI260AP 1.8 V power enable |
| P04 | `BOARD_XL9555_04_LORA_SEL` | `LORA_SEL` | Output | Schematic Page 3/7 | `HIGH` internal antenna, `LOW` external antenna |
| P05 | `BOARD_XL9555_05_MOTOR_EN` | `M_EN` | Output | Schematic Page 4/7 | DRV2605 power / enable |
| P06 | `BOARD_XL9555_06_AMPLIFIER` | `SHUTDOWM` | Output | Schematic Page 7/8 | Amplifier enable; the schematic net really is spelled `SHUTDOWM` |
| P07 | `BOARD_XL9555_07_TOUCH_RST` | `T_RST` | Output | Schematic Page 4/7 | Touch reset, active low |
| P10 | `BOARD_XL9555_10_PWRKEY_EN` | `PWRKEY_EN` | Output | Schematic Page 5/7 | A7682E `PWRKEY` control |
| P11 | `BOARD_XL9555_11_KEY_RST` | `KEY_RST` | Output | Schematic Page 7 | Keyboard reset, active low |
| P12 | `BOARD_XL9555_12_AUDIO_SEL` | `AUDIO_SEL` | Output | Schematic Page 7/8 | `HIGH = A7682E`, `LOW = ES8311` |
| P13 | `BOARD_XL9555_13` | NC | - | Schematic Page 7 | Reserved in current repo |
| P14 | `BOARD_XL9555_14` | NC | - | Schematic Page 7 | Reserved in current repo |
| P15 | `BOARD_XL9555_15` | NC | - | Schematic Page 7 | Reserved in current repo |
| P16 | `BOARD_XL9555_16` | NC | - | Schematic Page 7 | Reserved in current repo |
| P17 | `BOARD_XL9555_17` | NC | - | Schematic Page 7 | Reserved in current repo |

## 3. EPD Display

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| EPD DC | `BOARD_EPD_DC` | GPIO35 | Schematic Page 4 | Schematic net name `LCD_D/C` |
| EPD CS | `BOARD_EPD_CS` | GPIO34 | Schematic Page 4 | Schematic net name `LCD_CS` |
| EPD BUSY | `BOARD_EPD_BUSY` | GPIO37 | Schematic Page 4 | Schematic net name `LCD_BUSY` |
| EPD RST | `BOARD_EPD_RST` | GPIO9 | Schematic Page 4 | Schematic net name `LCD_RST` |
| EPD SCK | `BOARD_EPD_SCK` | GPIO36 | Schematic Page 4 | Shared main SPI |
| EPD MOSI | `BOARD_EPD_MOSI` | GPIO33 | Schematic Page 4 | Shared main SPI |
| EPD frontlight PWM | `BOARD_EPD_BL` | GPIO41 | Schematic Page 4 | Schematic net name `BL_PWM` |

## 4. CST328 Touch

I2C address: `0x1A`.

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| I2C SDA | `BOARD_TOUCH_SDA` | GPIO13 | Schematic Page 4 | Shared main I2C |
| I2C SCL | `BOARD_TOUCH_SCL` | GPIO14 | Schematic Page 4 | Shared main I2C |
| INT | `BOARD_TOUCH_INT` | GPIO12 | Schematic Page 4 | CST328 interrupt |
| RST | `BOARD_TOUCH_RST` | `XL9555 P07` | Schematic Page 4/7 | Board net `T_RST`, not directly wired to an ESP32 GPIO |

## 5. TCA8418 Keyboard

I2C address: `0x34`.

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| I2C SDA | `BOARD_KEYBOARD_SDA` | GPIO13 | Schematic Page 7 | Shared main I2C |
| I2C SCL | `BOARD_KEYBOARD_SCL` | GPIO14 | Schematic Page 7 | Shared main I2C |
| INT | `BOARD_KEYBOARD_INT` | GPIO15 | Schematic Page 7 | TCA8418 interrupt |
| Backlight PWM | `BOARD_KEYBOARD_LED` | GPIO42 | Schematic Page 7 | Schematic net name `LED_PWM` |
| RST | `BOARD_KEYBOARD_RST` | `XL9555 P11` | Schematic Page 7 | Board net `KEY_RST`, active low |

## 6. BHI260AP IMU

I2C address: `0x28`.

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| I2C SDA | `BOARD_GYROSCOPDE_SDA` | GPIO13 | Schematic Page 4 | Shared main I2C |
| I2C SCL | `BOARD_GYROSCOPDE_SCL` | GPIO14 | Schematic Page 4 | Shared main I2C |
| INT | `BOARD_GYROSCOPDE_INT` | GPIO21 | Schematic Page 4 | Schematic net name `HIRQ` |
| RST | `BOARD_GYROSCOPDE_RST` | `-1` | Schematic Page 4 | Reset pin is not connected in the current design |
| Power enable | `BOARD_XL9555_03_1V8_EN` | `XL9555 P03` | Schematic Page 4/7 | `HIGH` powers it on |

## 7. DRV2605 Motor

I2C address: `0x5A`.

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| I2C SDA | `BOARD_MOTOR_SDA` | GPIO13 | Schematic Page 4 | Shared main I2C |
| I2C SCL | `BOARD_MOTOR_SCL` | GPIO14 | Schematic Page 4 | Shared main I2C |
| Power / enable | `BOARD_MOTOR_EN` | `XL9555 P05` | Schematic Page 4/7 | Board net `M_EN` |

## 8. LoRa SX1262

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| SCK | `BOARD_LORA_SCK` | GPIO36 | Schematic Page 3 | Shared main SPI |
| MOSI | `BOARD_LORA_MOSI` | GPIO33 | Schematic Page 3 | Shared main SPI |
| MISO | `BOARD_LORA_MISO` | GPIO47 | Schematic Page 3 | Shared main SPI |
| CS | `BOARD_LORA_CS` | GPIO3 | Schematic Page 3 | Module `NSS` |
| RST | `BOARD_LORA_RST` | GPIO4 | Schematic Page 3 | Module `NRESET` |
| IRQ | `BOARD_LORA_INT` | GPIO5 | Schematic Page 3 | Module `DIO1` |
| BUSY | `BOARD_LORA_BUSY` | GPIO6 | Schematic Page 3 | Module `BUSY` |
| Power enable | `BOARD_XL9555_01_LORA_EN` | `XL9555 P01` | Schematic Page 3/7 | `HIGH` powers it on |
| Antenna select | `BOARD_XL9555_04_LORA_SEL` | `XL9555 P04` | Schematic Page 3/7 | Current repo assumes `HIGH` internal, `LOW` external |

## 9. GPS MIA-M10Q

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| PPS | `BOARD_GPS_PPS` | GPIO1 | Schematic Page 3 | Schematic net name `PPS` |
| Module TX -> MCU RX | `GPS_TX` / `BOARD_GPS_RXD` | GPIO2 | Schematic Page 3 | Net name follows the GPS module point of view |
| Module RX <- MCU TX | `GPS_RX` / `BOARD_GPS_TXD` | GPIO16 | Schematic Page 3 | Net name follows the GPS module point of view |
| Power enable | `BOARD_XL9555_02_GPS_EN` | `XL9555 P02` | Schematic Page 3/7 | `HIGH` powers it on |

## 10. A7682E 4G Modem

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| RI | `BOARD_A7682E_RI` | GPIO7 | Schematic Page 2/5 | Module `RI` |
| DTR | `BOARD_A7682E_ITR` | GPIO8 | Schematic Page 2/5 | In the current repo this macro is treated as `DTR` |
| Module RXD | `7682_RXD` / `BOARD_A7682E_RXD` | GPIO10 | Schematic Page 2/5 | Net name follows the module point of view |
| Module TXD | `7682_TXD` / `BOARD_A7682E_TXD` | GPIO11 | Schematic Page 2/5 | Net name follows the module point of view |
| Power enable | `BOARD_XL9555_00_6609_EN` | `XL9555 P00` | Schematic Page 1/7 | A7682E power domain |
| `PWRKEY` control | `BOARD_A7682E_PWRKEY` / `BOARD_XL9555_10_PWRKEY_EN` | `XL9555 P10` | Schematic Page 5/7 | Board net `PWRKEY_EN` |

## 11. TF Card

| Function | Current / Recommended Macro | GPIO | Source | Notes |
| --- | --- | --- | --- | --- |
| SD CS | `BOARD_SD_CS` | 48 | Schematic Page 2/6 | Shared SPI with EPD and LoRa |
| SD SCK | `BOARD_SD_SCK` | 36 | Schematic Page 3/6 | Shared main SPI |
| SD MOSI | `BOARD_SD_MOSI` | 33 | Schematic Page 3/6 | Shared main SPI |
| SD MISO | `BOARD_SD_MISO` | 47 | Schematic Page 3/6 | Shared main SPI |

## 12. ES8311 Audio

I2C address: `0x18`.

| Function | Current / Recommended Macro | GPIO / Mapping | Source | Notes |
| --- | --- | --- | --- | --- |
| I2C SDA | `BOARD_ES8311_SDA` | GPIO13 | Schematic Page 8 | Shared main I2C |
| I2C SCL | `BOARD_ES8311_SCL` | GPIO14 | Schematic Page 8 | Shared main I2C |
| I2S MCLK | `BOARD_ES8311_MCLK` | GPIO38 | Schematic Page 8 | `I2S_MCLK` |
| I2S BCLK/SCLK | `BOARD_ES8311_SCLK` | GPIO39 | Schematic Page 8 | `I2S_SCLK` |
| I2S LRCK | `BOARD_ES8311_LRCK` | GPIO18 | Schematic Page 8 | `I2S_LRCK` |
| ES8311 `ASDOUT` -> ESP32 DIN | Schematic net `I2S_ASDOUT` | GPIO40 | Schematic Page 8 | Named from the ESP32 point of view in the schematic |
| ESP32 DOUT -> ES8311 `DSDIN` | Schematic net `I2S_DSDIN` | GPIO17 | Schematic Page 8 | Named from the ESP32 point of view in the schematic |
| Amplifier enable | `BOARD_XL9555_06_AMPLIFIER` | `XL9555 P06` | Schematic Page 7/8 | `HIGH` enables it |
| Audio route select | `BOARD_XL9555_12_AUDIO_SEL` | `XL9555 P12` | Schematic Page 7/8 | `HIGH = A7682E`, `LOW = ES8311` |

> Note: `BOARD_ES8311_ASDOUT/DSDIN` in `readme*.md` and `examples/factory/utilities.h` uses the ESP32 I2S direction naming. If you interpret the names from the schematic net names directly, the corresponding mapping would be `ASDOUT = GPIO17` and `DSDIN = GPIO40`.

## 13. I2C Address Table

| Device | Address | Source | Notes |
| --- | --- | --- | --- |
| ES8311 | `0x18` | Schematic Page 8 | Audio codec |
| CST328 | `0x1A` | `readme*.md` | Touch controller |
| XL9555 | `0x20` | Schematic Page 7 | IO expander |
| BHI260AP | `0x28` | Schematic Page 4 | IMU |
| TCA8418 | `0x34` | Schematic Page 7 | Keyboard matrix controller |
| BQ27220 | `0x55` | Schematic Page 1 | Fuel gauge |
| DRV2605 | `0x5A` | Schematic Page 4 | Vibration motor driver |
| BQ25896 | `0x6B` | Schematic Page 1 | Charge management, no longer used |
| SY6970 | `0x6A` | Schematic Page 1 | Charge management |

Note: `BQ25896` has been disabled for later hardware revisions, and later `T-Deck-MAX` boards use `SY6970`.

## 14. Current Codebase Notes

- In the current repository, `BOARD_A7682E_ITR` should be understood as `DTR`.
- For GPS, A7682E UART, and ES8311 I2S, the codebase mixes two naming perspectives: peripheral-net naming and ESP32-side naming. When writing new code, it is best to state the direction explicitly.
- Board macros are currently scattered across `readme*.md`, `examples/factory/utilities.h`, and several example-specific `utilities.h` files. It would be better to consolidate them into a single shared header in the future.
