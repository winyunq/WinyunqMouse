# 云指鼠标 / WinyunqMouse

云指鼠标-质变小的鼠标

云指鼠标是我本科毕业时的毕设。因为现在 AI 的能力大幅度提升，我终于能打磨它了。

这个分支，我们的目的只有一个：让大家真正的能用上云指鼠标。

## 代码

[项目源码](src/)

保留原有 `src/`、`libs/` 和 TMOS/BLE HID 架构。
[架构说明](src/Architecture.dox) · [PAW3805EK 驱动说明](src/PAW3805EK.dox)

已加入 **PAW3805EK-CJV1 裸芯片驱动**：三线 GPIO SPI、产品 ID 检查、TMOS 非阻塞复位等待、16 位 XY 读取和掉电命令。位移接入原 BLE HID 上报，保留原按键、运行配置和霍尔代码（`UsingPAW3805EK=0` 可选回旧路径）。核心接口与流程使用 Doxygen 注释。

### 配置与编译

1. 在本机浏览器打开 [config/index.html](config/index.html)，选择实际连接的 NCS、SCLK、SDIO GPIO 和方向，下载配置头文件。
2. 用它替换 `src/include/PAW3805EKConfig.h`。默认文件 **未配置引脚、不访问传感器 GPIO**，不能直接烧录后期待鼠标移动。
3. 使用兼容的 WCH RISC-V GCC 工具链构建，例如：

   ```sh
   make PREFIX=/path/to/riscv-none-embed- -j2
   ```

保留原默认 CH59x/CH592 目标、60 MHz 时钟和链接布局。**原 makefile 默认选择 OTA 链接脚本，入口位于 `0x1000`**；烧录前必须确认板卡、引导程序和固件布局，不把编译成功当作可直接刷入任意开发板。引脚编号不是封装脚号；裸芯片还需要正确供电、去耦和配套镜片，不能使用原霍尔供电 GPIO 代替电源设计。

### 验证状态

- 已通过 GPIO 事务/状态机测试、八种坐标方向与 HID 分帧/重试测试、配置页面生成逻辑测试；[运行测试](tests/README.md)。
- 已用 WCH GCC 8.2 完成 CH592 目标编译、链接及 HEX 生成，包含未配置和示例引脚配置两种构建。示例引脚仅用于编译测试。
- **尚未连接真实芯片**：SPI 最小时序不完整，当前可调延时是工程选择，仍需逻辑分析仪及真机确认通信、BLE 重连、手感和功耗。未实现 CPI 调节，也未完成整机电源设计。

寄存器协议依据明确标注该完整型号的 [Infineon 官方参考资料](https://github.com/Infineon/Code-Examples-BT-SDK-for-ModusToolbox/blob/fb65fbe6fbc0f0daa9b18837354407148d554acc/CYW920820EVB-02/apps/demo/hid/ble_mouse/motion/PAW3805_opticalsensor.c)，本驱动独立编写，不复制其受限许可代码。

## 硬件

### 当前参数

以下区分**当前代码配置**和**传感器标称参数**，不是整机实测规格。

| 项目 | 参数 |
| --- | --- |
| 软件平台 | CH59x / CH592，当前代码主频 60 MHz；实物开发板型号待确认 |
| 计划使用的传感器 | PAW3805EK-CJV1 |
| 传感器接口 | 三线 SPI：NCS、SCLK、双向 SDIO，最高 2 MHz；X/Y 位移支持 16 位 |
| 传感器标称分辨率 | 最高 3000 CPI；不代表当前固件已实现该配置 |
| 传感器封装尺寸 | 10.7 × 9.8 × 3.67 mm，不含外置镜片、PCB、电池和外壳 |
| 传感器包络体积估算 | 10.7 × 9.8 × 3.67 ≈ **384.84 mm³ ≈ 0.385 cm³**；只是外接长方体估算 |
| 传感器典型运行功率 | 3.3 V × 2.5 mA = **8.25 mW**，含光源 LED，不是最大值 |
| 传感器典型掉电模式功率 | 3.3 V × 15 μA = **49.5 μW**，不是整机待机功耗 |
| 整机最大功耗 | **待确定**：缺少适用工况的最大电流、完整电源方案和实测数据 |
| 整机尺寸 / “指腹大小”的体积 | **待确定**：仓库尚无 PCB、外壳或 3D 模型尺寸，不能拿传感器尺寸代替整机 |

传感器数据来自 [PixArt General Datasheet v1.3，第 1、6–8 页](https://www.codico.com/de/mpattachment/file/download/id/951/)。
典型电流条件为 3.3 V、25°C、玻璃表面；运行电流按手册的运动速度权重统计。
上述功率是 `P = U × I` 的估算，不是测量值；整机还要计入 MCU、指示灯及电源损耗。
MCU 参考资料：[WCH CH592 系列原厂手册 v1.9](https://github.com/openwch/ch592/blob/a46e0086f1ffb5e5502703970bff94888e67f4cb/Datasheet/CH592DS1_EN.PDF)。

## 3D模型

待补充。
