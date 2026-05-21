# pico-celeste

本 Readme 使用 Deepseek 辅助生成  

PICO-8 版《Celeste》的纯 C 语言移植，可在 ESP32 和桌面系统（SDL2）上运行。

## 项目简介

本工程将 PICO-8 版 Celeste 的原版 Lua 代码（`celeste.lua`，约 1434 行）完整翻译为 C 语言，同时实现了一层 PICO-8 图形接口模拟层，复刻了 `spr`、`map`、`rectfill`、`circfill`、`print`、`pal`、`camera`、`btn` 等 API，使游戏逻辑代码与原版保持一致。

核心特性：
- **128×128** 虚拟像素屏幕，还原 PICO-8 原生分辨率
- 全局使用 **Q16.16 定点数**（基于 `libfixmath`），模拟 PICO-8 数值系统
- 面向对象设计：通过函数指针实现虚方法表（`type_t`），结构体内嵌基类实现继承
- 1:1复制了游戏内容：角色移动、冲刺、蹬墙跳、尖刺碰撞、关卡加载、卷轴云/粒子、屏幕震动、水果收集、假墙、弹簧、气球、坠落平台、钥匙/宝箱、双冲球、终点旗杆及通关后数据统计

## 平台支持

| 目标平台 | 入口文件 | 构建系统 | 渲染方式 | 输入方式 |
|---------|---------|---------|---------|---------|
| **ESP32** | `main/pico-celeste.c` → `app_main()` | ESP-IDF CMake | ST6201 LCD (SPI, 480×272, RGB565) | GT911 触摸IC，映射 6 个虚拟按键 |
| **桌面 (Linux/macOS/Windows)** | `sdl-host/main.c` → `main()` | CMake + SDL2 | SDL2 窗口 (512×512, 4 倍缩放) | 键盘（方向键 + C 跳跃 + X 冲刺） |

### ESP32 硬件配置

| 项目 | 参数 |
|-----|-----|
| MCU | ESP32 |
| LCD 驱动 | ST6201, 480×272, RGB565 |
| LCD 接口 | SPI2 (MOSI: GPIO19, SCLK: GPIO23, DC: GPIO14, CS: GPIO22, RST: GPIO12) |
| 触摸面板 | GT911, I2C (SDA: GPIO18, SCL: GPIO16) |
| 屏幕布局 | 128×128 游戏画面居中绘制于物理屏幕，偏移 (176, 73) |
| 帧率 | 30 FPS（33ms 每帧） |
| 背光控制 | GPIO2 |

## 目录结构

```
pico-celeste/
├── celeste.lua                  # PICO-8 原始 Lua 源码（参考）
├── Emulator.cs                  # 蔚蓝官方的C# PICO-8 模拟器参考实现
├── 笔记.txt                     # 设计笔记：对象生命周期、模板模式说明
│
├── main/                        # 游戏核心源码（双平台共享）
│   ├── CMakeLists.txt           # ESP-IDF 组件注册
│   ├── idf_component.yml        # ESP-IDF 组件依赖
│   ├── pico-celeste.c           # ESP32 入口：LCD/触摸初始化，主循环
│   ├── classic.c                # 游戏逻辑（约 1964 行）
│   ├── classic.h                # 类型定义：实体结构体、枚举
│   ├── pico8sim.c               # PICO-8 图形 API 模拟实现
│   ├── pico8sim.h               # PICO-8 模拟层头文件
│   ├── sprite.c                 # 精灵表数据（编译为二进制数组）
│   ├── tilemap.c                # 关卡瓦片地图数据
│   └── classiceleste_bg.c       # LCD 背景图片（480×272, RGB565）
│
├── sdl-host/                    # 桌面 SDL2 目标
│   ├── CMakeLists.txt           # 独立 CMake 构建
│   ├── main.c                   # SDL 入口：窗口创建、渲染循环、输入
│   ├── platform.c               # SDL 键盘 → btn() 映射
│   ├── platform.h               # 平台输入接口
│   └── compat.h                 # 兼容性宏（替代 FreeRTOS）
│
├── components/                  # ESP-IDF 自定义组件
│   ├── esp_lcd_st6201/          # ST6201 LCD 面板驱动
│   └── libfixmath/              # Q16.16 定点数运算库（sin、cos、sqrt 等）
│
├── managed_components/          # ESP-IDF 组件管理器自动下载的依赖
│   ├── espressif__cmake_utilities/
│   ├── espressif__esp_lcd_touch_gt911/
│   └── espressif__esp_lcd_touch/
│
└── CMakeLists.txt               # 根 CMake（ESP-IDF 工程文件）
```

## 依赖

| 依赖 | 作用 | 范围 |
|-----|-----|-----|
| ESP-IDF v6.0.1+ | RTOS、硬件抽象、SPI/I2C 驱动 | ESP32 专用 |
| libfixmath | Q16.16 定点数运算（用于模拟pico8的数值类型） | 双平台 |
| esp_lcd_st6201 | ST6201 LCD 驱动 | ESP32 专用 |
| esp_lcd_touch_gt911 | GT911 触摸驱动（托管组件） | ESP32 专用 |
| SDL2 | 窗口、渲染、输入 | SDL 宿主专用 |

## 构建与运行

### ESP32（使用 ESP-IDF）

```bash
# 设置目标芯片
idf.py set-target esp32

# 编译并烧录
idf.py build flash monitor
```

### 桌面 SDL2

```bash
# 安装 SDL2 依赖

# 构建与运行
cd sdl-host
cmake --build build
./build/sdl-celeste
```

## 内部架构

### 对象生命周期

通过 `init_object()` 和 `destroy_object()` 管理游戏对象（最大 50 个）。`_update()` 在一次遍历中完成：标记无效对象、执行更新、压缩数组，保持 O(n) 时间复杂度。

### 模板类型系统

所有游戏实体遵循统一的结构模式：

```c
// 类型定义（虚方法表）
typedef struct type_s {
    uint8_t tile;
    uint8_t if_not_fruit;
    size_t size;
    void (*init)(obj_t* obj);
    void (*update)(obj_t* obj);
    void (*draw)(obj_t* obj);
} type_t;

// 实体结构体（obj_t 作为"基类"位于首位）
typedef struct TEMPLATE_s {
    obj_t obj;        // 必须放在第一位，确保指针可直接转换
    // ... 实体特有字段 ...
} TEMPLATE_t;
```

对象种类：
- **由其他object创建**：`player`、`smoke`、`fruit`、`lifeup`、`orb`、`platform`、`room_title`
- **地图加载时搜索并创建**：`player_spawn`、`spring`、`balloon`、`fall_floor`、`fly_fruit`、`fake_wall`、`key`、`chest`、`message`、`big_chest`、`flag`

### PICO-8 模拟层

`pico8sim.c` 实现了 PICO-8 图形接口的核心函数，包括：
- 精灵绘制（支持翻转、透明色）
- 瓦片地图渲染
- 基本图形绘制（矩形填充、圆形填充、线条）
- 文字输出
- 调色板重映射
- 屏幕震动偏移（camera）
- 按钮输入

## 参考来源

- [PICO-8 Celeste 原始 Lua 源码](https://www.lexaloffle.com/bbs/?tid=2145)
- Celeste 作者在 Monocle 引擎实现的 C# PICO-8 模拟器（`Emulator.cs`）
- Celeste classic 原作 由 Maddy Thorson & Noel Berry 开发

## 许可

本项目仅供学习研究使用。Celeste 游戏版权归原作者所有。

# todo
金草莓  
预输入  
抓墙  
冲刺 跳跃加速
