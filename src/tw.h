#ifndef __COMMON_TW_H__
#define __COMMON_TW_H__
#include <functional>

/*
//初始化
*/
void tw_init();

/*
//回收
*/
void tw_destroy();

/*
//获取时间轮支持的最大定时时长，毫秒
*/
uint64_t tw_max_ms();

/*
//获取当前时间戳，毫秒
*/
uint64_t get_current_ms();

/*
//获取当前时间戳，微秒
*/
uint64_t get_current_us();

/*
//添加计时器
    ms:   定时器时长，毫秒
    func: 回调函数
    args: 回调参数
*/
uint64_t tw_add_timer(int64_t ms, std::function<void(uint64_t,void*)> func, void *args);

/*
//删除计时器
    id: 计时器ID
*/
void tw_del_timer(uint64_t id);

/*
//休眠
    _usl： 是否使用select
*/
void tw_sleep_us(int64_t us, bool _usl);

#endif