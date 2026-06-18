#include <math.h>
#include <stdio.h>

#define SAMPLE_RATE 100
#define WINDOW_SIZE 200
#define FILTER_ALPHA 0.15f
#define PEAK_THRESH 0.8f
#define MAX_PERIOD_BUF 10

float acc_buf[WINDOW_SIZE];
float period_buf[MAX_PERIOD_BUF];
uint16_t buf_idx = 0, p_buf_idx = 0;
float last_filter_mag = 0.0f;
float last_peak_t = 0.0f;

// 一阶低通滤波
float LPF(float raw, float last)
{
    return FILTER_ALPHA * raw + (1 - FILTER_ALPHA) * last;
}

// 三轴合加速度
float AccMag(float ax, float ay, float az)
{
    return sqrtf(ax*ax + ay*ay + az*az);
}

// 判断峰值
uint8_t IsPeak(int idx, float *buf, int len)
{
    if(idx <= 0 || idx >= len-1) return 0;
    if(buf[idx] > buf[idx-1] && buf[idx] > buf[idx+1] && buf[idx] > PEAK_THRESH)
        return 1;
    return 0;
}

// 单次采样更新，返回当前抖动频率
float CalcShakeFreq(float ax, float ay, float az, float cur_t)
{
    // 滤波合加速度
    float mag = AccMag(ax, ay, az);
    float f_mag = LPF(mag, last_filter_mag);
    last_filter_mag = f_mag;

    // 滑动窗口存储
    acc_buf[buf_idx++] = f_mag;
    if(buf_idx >= WINDOW_SIZE) buf_idx = 0;

    // 遍历窗口找峰值
    for(int i = 0; i < WINDOW_SIZE; i++)
    {
        if(IsPeak(i, acc_buf, WINDOW_SIZE))
        {
            float dt = cur_t - last_peak_t;
            if(dt > 0.02f)
            {
                period_buf[p_buf_idx++] = dt;
                if(p_buf_idx >= MAX_PERIOD_BUF) p_buf_idx = 0;
            }
            last_peak_t = cur_t;
        }
    }

    // 计算平均频率
    float sum_T = 0;
    for(int i = 0; i < MAX_PERIOD_BUF; i++) sum_T += period_buf[i];
    float avg_T = sum_T / MAX_PERIOD_BUF;
    return 1.0f / avg_T;
}