# zigbee-vw-sensor

八通道振弦传感器 Zigbee 无线采集节点 —— 用于土木工程结构监测（大坝、桥梁、隧道等）的工业 IoT 设备。

## 硬件架构

| 模块 | 型号 | 功能 |
|------|------|------|
| MCU | STM32L051C8Tx | Cortex-M0+, 64KB Flash, 8KB RAM |
| 振弦采集 | VM101 | 单通道频率+温度 Modbus RTU 采集 |
| 通道切换 | GPIO PB0-PB7 | 8 路外部模拟开关，分时复用 |
| Zigbee | DRF1609 | 2.4GHz 无线，HY65/ZDYH 协议 |
| 433MHz 遥控 | E330 | 超低功耗远程唤醒接收 |
| RS485 | SP3485 | 有线 Modbus 通信 |
| 充电管理 | TP4057 | 单节锂电池充电 (500mA) |
| 升压 | MT3608 | 电池 → 5V |
| Buck | LGS5148 | 24V → 5.5V (48V 耐压) |
| LDO × 4 | XC6204/6206/6210 | 5V / 3.3V × 2 / Zigbee 独立供电 |

### 电源架构

```
24V DC ──→ [Fuse] ──→ LGS5148 ──→ 5.5V ──→ Q7 ──→ 5VBus ──┬── XC6206 ──→ LDO5V  (VM101)
                                                              │
USB 5V ──→ TP4057 ──→ Battery ──→ MT3608 ──→ 5V ──→ Q6 ──┘   ├── XC6204 ──→ +3V3   (STM32)
                                                              ├── XC6204 ──→ 3.3V   (RS485/Relays)
                                                              └── XC6210 ──→ VZIGBEE (DRF1609)
```

## 固件架构

```
main.c
  └── app_controller.c      ← 顶层状态机
        ├── ch8_measure.c       ← 8 通道采集 (Modbus RTU)
        ├── zdyh_cmd.c          ← ZDYH 40 字节二进制协议
        ├── hy65_ascii_cmd.c    ← 65TX/65RX ASCII 协议
        ├── hy65_protocol.c     ← 帧构造 / 校验和
        ├── wireless_board.c    ← GPIO 电源管理 / STOP 模式
        ├── mbslave.c           ← Modbus CRC / 接收状态机
        └── app_config.c        ← Flash 参数持久化
```

### 状态机

```
FIRST_RUN → JUDGE_REMOTE_MSG → SYS_INIT → RUN_MODE
    ↑            ↓                          │
    └────────────┴────────── 超时休眠 ──────┘
```

- **FIRST_RUN**: 检查 433MHz 遥控模块，进入 STOP 模式等待唤醒
- **JUDGE_REMOTE_MSG**: 收到唤醒后解析 `CMD:XXXX\r\n` 指令
- **SYS_INIT**: 依次上电 Zigbee → 传感器 → VM101，初始化 UART
- **RUN_MODE**: 主循环 —— 测量 / Zigbee 协议处理 / 透传中继

### 工作模式

| Mode | 输出 | 计算公式 |
|------|------|----------|
| 0 | 应变 (με) | `(f^2/1000 - f0^2/1000) × 10 × K × C` |
| 1 | 频模 (Hz^2/1000) | `f × f/100 × C` |
| 2 | 频率 (0.1Hz) | `f × C` |

## 通信协议

### ZDYH 40 字节二进制协议 (Zigbee)

```
 0-3: "ZDYH"         Header
 4-5: 0x28 0x00      Frame size (40)
 6-9: u32            XOR checksum
10-11: 0x00 0x01     Hardware type & version
12-15: u32           Device ID
16-17: 0x0F 0x01     Sensor type (strain gauge) & range
  18: 0x00           Reserved
  19: u8             Error code
  20: u8             Group address
  21: u8             Command (0x06=query, 0x04=set zero, etc.)
22-25: u32           Timestamp
26-29: u32           Baud rate / parameter
30-31: u16           COM config & battery level
32-33: 0x0A 0x3F     Internal flags
34-35: u16           Temperature
36-39: s32           Measurement value
```

### 65TX/65RX ASCII 协议 (RS485/兼容)

```
请求: 65TXnnnC + checksum
响应: 65RXnnnCOsdddddd0 + checksum

nnn  = 节点号 (ASCII)
C    = S(读数) T(温度) Z(清零) B(波特率) C(校准) F(频率) K(K值) M(模式)
```

### Modbus RTU (VM101)

- 9600 baud, 8N1
- 功能码 0x03 读保持寄存器
- 频率 + 温度，每通道 2 个 16-bit 寄存器

### 433MHz 遥控唤醒

- 格式: `CMD:XXXX\r\n` (X = 0/1，对应 4 组通道)
- 唤醒后进入 SYS_INIT，空闲 30 分钟自动休眠

## Flash 参数存储

基地址 `0x08080000` (Flash Page 128):

| 偏移 | 参数 | 默认值 |
|------|------|--------|
| +0x00 | 有效标志 | 0xA5 |
| +0x04 | 设备 ID | 0x253FF601 |
| +0x08 | 分组号 | 1 |
| +0x0C | 校准系数 | 0 |
| +0x14 | 工作模式 | 2 (频率) |
| +0x18 | 中心频率 | 800 |
| +0x20 | K 值 (G 值) | 3.7 |
| +0x24 | 零点基准 [8] | — |

## 开发环境

- **IDE**: Keil MDK-ARM V5 (ARMCC V5.06)
- **HAL**: STM32Cube FW_L0 V1.12.1
- **CubeMX**: 6.2.0 (`.ioc` 项目文件)
- **MCU Package**: Keil.STM32L0xx_DFP.3.0.0

## 目录结构

```
├── firmware/                    STM32 固件工程
│   ├── Core/Inc/                应用层头文件
│   ├── Core/Src/                应用层源文件
│   ├── Drivers/                 HAL + CMSIS
│   ├── MDK-ARM/                 Keil 工程 + 启动文件
│   └── STM32L051_8CH.ioc       CubeMX 配置
├── INC_unzip/INC/              HY65/HYDZ 协议头文件
│   ├── HY65ASCII_CLIENT.h      65TX/65RX ASCII 协议定义
│   ├── HYDZHEAD.H              40 字节报文头、产品类型
│   ├── HYDZFILE.H              文件格式定义
│   ├── Hy65client.h            HY65 传感器客户端结构
│   ├── Hy65file.h              HY65 文件格式
│   └── HYDZCHECKSUM.c          校验和实现
├── work/                       Python 工具 + 数据
│   ├── parse_history_docs.py   解析 EasyEDA 项目历史
│   ├── analyze_ch8.py          PCB 元件分析
│   ├── ch8_power_table.py      电源网络分析
│   ├── decrypt_eprj2_history.py AES-GCM 解密
│   ├── inspect_eprj2.py        项目文件探查
│   ├── project_structure.py    项目结构读取
│   ├── export_history_blob.py  历史数据导出
│   ├── probe_history.py        历史数据诊断
│   ├── decrypt_history_node.js Node.js 版解密
│   └── firmware_backup/        旧版固件快照 (before_ch8)
└── outputs/                    输出产物
    ├── ch8_logic_diagram.svg   PCB 逻辑框图
    └── ch8_logic_diagram.png
```

## Changelog

### v2.0.0 (2026-07-02) — 重大重构

**从 4 通道 → 8 通道，单文件 → 模块化架构**

| 方面 | v1.x (VW1004) | v2.0 (VM101) |
|------|---------------|--------------|
| 通道数 | 4 | **8** |
| 代码结构 | 单文件 `main.c` ~2200 行 | **12 个模块** |
| 通道切换 | VW1004 内部 | GPIO PB0-PB7 外部模拟开关 |
| 测量方式 | 单次读取 4 路 | 逐通道分时采集 |
| 通道验证 | 硬编码 4 路比较 | `CH8_IsLocalChannelId()` 范围检查 |
| 广播响应 | 手动 bitmask | `CH8_NextPendingChannelIndex()` |
| 拨码开关 | PB6-PB9 DIP 组地址 | **已移除** |
| 状态机 | 内联 switch | 独立 `app_controller` 模块 |
| 无线管理 | `main.c` 内静态函数 | `wireless_board` 模块 |

**新增模块**:
- `app_controller.c` — 顶层状态机
- `ch8_measure.c` — 8 通道采集
- `zdyh_cmd.c` — ZDYH 协议处理
- `hy65_ascii_cmd.c` — 65TX/65RX ASCII 协议
- `hy65_protocol.c` — 帧构造 & 校验
- `wireless_board.c` — GPIO 电源管理

## License

Copyright © Ccmra
