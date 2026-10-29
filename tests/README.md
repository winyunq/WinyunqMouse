# 主机测试与目标构建

在仓库根目录运行（需要 C 编译器及 Node.js）：

```sh
sh tests/PAW3805EK/run.sh
sh tests/optical_integration/run.sh
node tests/config_gui.cjs
```

- `PAW3805EK`：真实驱动源文件配合 GPIO/TMOS 模型，检查 Mode 3 时钟与 SDIO 方向、地址/数据、ID 探测、8 tick 复位等待、符号位移、掉电及未配置/非法 GPIO 门禁。
- `optical_integration`：编译真实 `WinyunqMouse.c`，以替身传感器与 BLE 接口检查全部八种方向组合、正常余量分帧、1016 counts 上界、成功排空/失败退避、断连清理和按键重同步；启用 UBSan。
- `config_gui.cjs`：执行离线页面中的真实脚本，检查空配置、重复/占用 GPIO、方向开关及生成头文件，共 32 个断言；这是逻辑测试，不是浏览器视觉测试。

目标构建使用 WCH GCC 8.2（`riscv-none-embed-`），不是普通桌面 GCC。可按 README 配置 `PREFIX`，默认 `BoardVersion=CH59x`。已验证安全未配置构建及通过构建宏传入示例引脚的配置构建；不提交测试 HEX，示例引脚不代表已确认的接线。另已通过 `UsingPAW3805EK=0` 的旧霍尔构建及 `UsingOTA=` 的非 OTA 构建（入口 `0x0`），但未改变默认布局。

这些检查不模拟实际电气时序，也不运行完整 WCH BLE 协议栈。`hidmouse.c` 的通知/重连回调经过源码复核和目标编译，仍需真机动态测试。无实物烧录、传感器运动、最大功耗或整机尺寸验证。当前环境未安装 Doxygen，因此未生成或检查 HTML 文档渲染。
