# 🦞 ketao88 定制说明

基于上游 [espressif/esp-claw](https://github.com/espressif/esp-claw) 的定制 fork，适配 `esp32_S3_DevKitC_1_breadboard` 硬件。

## 改动内容

| # | 改动 | 说明 |
|---|------|------|
| 1 | 🔧 修复 WiFi 连接 | 解决一直等待连接的问题 |
| 2 | 💡 WS2812 状态指示 (GPIO 48) | 红色闪烁 = 等待网络 / 绿色常亮 = WiFi已连接 / 紫色慢闪 = 连接大模型中 |
| 3 | ⚡ Token 优化 | 降低 token 消耗，减少延迟 |
| 4 | 🧹 移除 cap_hid_keyboard | 依赖的 esp_tinyusb 组件不可用，临时禁用 |
| 5 | 🧠 修复 IRAM 溢出 | 启用 SRAM1 作为 IRAM（+32KB） |
| 6 | 📦 CI 自动打包 | push 代码后自动编译并生成单文件 firmware_full.bin |

## 下载固件

GitHub Actions 构建完成后，进入 Actions 页面，选择最新成功的运行记录，
底部 Artifacts 栏下载 `esp-claw-firmware.zip`，解压得到 `firmware_full.bin`。

刷机方式：
- 网页刷机：上传 `firmware_full.bin` 即可
- esptool：`esptool.py write_flash 0x0 firmware_full.bin`
