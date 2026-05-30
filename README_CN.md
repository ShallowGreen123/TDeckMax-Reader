# T-Deck-Max CrossPoint 移植说明

[English](./READMD.md) | `中文`

## 1. 致谢与项目改动

首先感谢 [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) 开源项目的作者和贡献者。
这个项目为墨水屏阅读器提供了完整、可读、可扩展的开源固件基础，也让后续面向其他硬件平台的移植成为可能。

当前仓库基于 CrossPoint 的设计思路和阅读器框架，面向 **LilyGO T-Deck-Max** 做了适配和扩展。已经完成的主要改动包括：

- 将原本面向 Xteink X4 的固件移植到 **T-Deck-Max / ESP32-S3** 平台。
- 适配 T-Deck-Max 的 **240 x 320 墨水屏**、键盘矩阵、供电管理、充电与电量检测。
- 重建输入层，把设备物理键盘映射为阅读器需要的 `Back / Confirm / Left / Right / Up / Down / Power` 逻辑按键。
- 新增并完善 **键盘映射设置**，允许在 `Settings -> Controls -> Keyboard Mapping` 中自定义按键。
- 增加了 **Battery Status** 页面，用于查看电池、充电和电源状态。
- 对界面、列表、阅读页布局和主题做了针对 T-Deck-Max 小尺寸屏幕的适配。
- 保留并适配了 CrossPoint 原有的阅读能力，包括 **EPUB / TXT / XTC** 阅读、缓存、进度保存、屏幕旋转、状态栏配置等。

说明：
这个仓库已经是 **T-Deck-Max 专用版本**，不再以兼容原始 X4 硬件为目标。

## 2. T-Deck-Max 硬件参数

以下为当前移植版本重点用到的硬件参数：

在 T-Deck-Max 仓库查看: [github](https://github.com/Xinyuan-LilyGO/T-Deck-MAX)

crosspoint 固件下载查看：[./firmware](./firmware/README.md)

![Tdeckmax](./docs/TDeckMax/image1.png)




## 3. 基本使用说明

### 3.1 开关机与主界面

- 长按 `BOOT` 键关机，长按 `PWR` 键开机。
- 启动后默认进入主界面 `Home`。
- 主界面可以进入文件浏览、最近阅读、文件传输、设置等功能。

### 3.2 导入图书

常见使用方式有两种：

- 将书籍拷贝到存储中，然后在 `Browse Files` 中打开。
- 通过 Wi-Fi 文件传输页面上传书籍。

当前固件主要面向以下格式：

- `EPUB`
- `TXT`
- `XTC`

### 3.3 设置入口

设置页面主要入口：

- `Settings -> Display`
- `Settings -> Reader`
- `Settings -> Controls`
- `Settings -> System`

和 T-Deck-Max 关系最密切的设置包括：

- `Settings -> Controls -> Keyboard Mapping`
- `Settings -> Controls -> Battery Status`
- `Settings -> Reader -> Customise Status Bar`

## 4. 默认按键映射说明

当前默认逻辑按键映射如下：

| 逻辑功能 | T-Deck-Max 默认按键 |
| --- | --- |
| Back | `Del` |
| Confirm | `Enter` |
| Left | `A` |
| Right | `D` |
| Up | `W` |
| Down | `S` |
| Power | `BOOT` |

在大多数菜单页面中：

- `W` 或 `A`：上一个 / 向上
- `S` 或 `D`：下一个 / 向下
- `Enter`：确认 / 进入
- `Del`：返回

说明：

- 默认情况下，阅读页还会把 `W / S` 作为翻页逻辑键使用。
- 如果你不喜欢默认布局，可以在 `Settings -> Controls -> Keyboard Mapping` 中重新绑定。

## 5. 阅读页面操作

不同格式的阅读页操作略有区别，下面按实际行为说明。

### 5.1 EPUB 阅读页

基本操作：

- `A`：上一页
- `D`：下一页
- `W`：上一页
- `S`：下一页
- 短按 `Del`：返回首页
- 长按 `Del` 约 1 秒：回到文件浏览器
- 按 `Enter`：打开阅读菜单

如果启用了 `Settings -> Controls -> Long Press Skip`：

- 长按翻页键后松开，可直接跳到上一章或下一章

EPUB 阅读菜单中可用的功能包括：

- 章节选择
- 脚注列表
- 跳转百分比
- 自动翻页
- 屏幕方向切换
- 截图
- 显示当前页二维码文本
- 返回首页
- KOReader Sync 进度同步
- 删除本书缓存

补充：

- 如果当前页来自脚注跳转，短按 `Del` 会优先返回原来的正文位置。
- 全局截图快捷键为 `BOOT + S`。

### 5.2 TXT 阅读页

基本操作：

- `A`：上一页
- `D`：下一页
- `W`：上一页
- `S`：下一页
- 短按 `Del`：返回首页
- 长按 `Del` 约 1 秒：回到文件浏览器

TXT 阅读页目前更偏向纯文本顺序阅读，不提供 EPUB 那样的完整菜单能力。

### 5.3 XTC 阅读页

基本操作：

- `A`：上一页
- `D`：下一页
- `W`：上一页
- `S`：下一页
- 短按 `Del`：返回首页
- 长按 `Del` 约 1 秒：回到文件浏览器
- 按 `Enter`：打开章节选择

如果启用了 `Settings -> Controls -> Long Press Skip`：

- 长按翻页键可按更大的步进翻页

### 5.4 电源键相关

`BOOT` 键既是电源键，也是逻辑上的 `Power` 键。

- 长按可关机
- 在系统设置中，可以把短按电源键配置为：
  - 忽略
  - 阅读页翻页
  - 强制刷新

## 6. 使用建议

- 第一次打开较大的 EPUB 时，系统会建立缓存，这是正常现象。
- 如果阅读排版异常，可以尝试删除阅读缓存后重新打开。
- 如果需要自定义操作习惯，优先调整：
  - `Keyboard Mapping`
  - `Short Power Button`
  - `Long Press Skip`
  - `Customise Status Bar`

---
