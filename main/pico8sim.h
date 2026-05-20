#ifndef PICO8SIM_H
#define PICO8SIM_H

#include "fixmath.h"
#include "stdarg.h"

//基础入口
void _update();
void _draw();
void _init();

void p8blit_to_rgb565(uint16_t *buf);

//底层接口
//所有涉及**编号 **掩码 **判断 使用 uint8_t
//其余都用 fix16_t

uint8_t fget(uint8_t n, uint8_t f); // 获取精灵 N 的标记 F 的值（VAL）。

void print(const char* fmt, fix16_t x, fix16_t y, uint8_t col, ...);
void camera(fix16_t x, fix16_t y); //为所有绘图操作设置屏幕偏移量为 -x, -y
void rectfill(fix16_t x0, fix16_t y0, fix16_t x1, fix16_t y1, uint8_t col);//绘制一个填充矩形
void circfill(fix16_t x,fix16_t y,fix16_t r,uint8_t col);//在坐标 (x, y) 处绘制一个半径为 r 的实心圆。如果 r 为负数，则不绘制该圆形。
void pal(uint8_t c0, uint8_t c1); //将颜色 c0 替换为 c1，作用于绘制调色板重映射
//在位置 X,Y 处绘制精灵 N（编号 0..255）
//精灵的大小为8*8,索引为sprite[n*64+i] 连续64个字节
//颜色 0 默认绘制为透明
//当 FLIP_X 为 TRUE 时，水平翻转。
//当 FLIP_Y 为 TRUE 时，垂直翻转。
void spr(uint8_t n, fix16_t x0, fix16_t y0,uint8_t flip_x, uint8_t flip_y);
//获取按钮 B 状态
//B: 0..5：左 右 上 下 按钮 O 按钮 X
uint8_t btn(uint8_t b);
void sfx(uint8_t n); //播放音效 N
//从模式 N（0..63）开始播放音乐
//FADE_LEN 单位为毫秒（默认值：0）。
//CHANNEL_MASK 指定哪些通道仅保留给音乐使用
void music(int16_t n, int16_t fade_len, uint8_t channel_mask); 
uint8_t mget(int16_t x, int16_t y); //获取地图上 X,Y 位置的数值 当 X 和 Y 超出边界时，MGET 返回 0
void map(int16_t tile_x, int16_t tile_y, int16_t sx, int16_t sy, int16_t tile_w, int16_t tile_h, uint8_t layers); //在地图位置（从 TILE_X, TILE_Y 开始）绘制指定区域到屏幕位置 SX, SY（像素单位）。

fix16_t rnd(fix16_t n);
#endif