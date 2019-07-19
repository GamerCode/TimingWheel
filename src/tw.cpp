#include <iostream>
#include <sys/types.h>
#include <functional>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <cmath>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include "dlink.h"
#include "tw.h"

using std::cout;
using std::cin;
using std::endl;


/*
时间轮实现
 */

using tw_callback_func = std::function<void(uint64_t,void*)>;

#define TW_WHEEL_SLOT_COUNT 20
#define TW_MAX_WHEEL_LEVEL 10
std::atomic_uint64_t _global_id(1);

//任务
struct tw_timer_t:public dlinklist_node_t
{
    uint64_t id=0;
    uint64_t ms=0;
    tw_callback_func callback;
    void *data=nullptr;
};

//轮子
struct tw_wheel_t
{
    uint8_t idx=0;  //当前位置
    uint32_t slot_size=0;
    uint64_t tick=0;
    uint64_t max=0;
    dlinklist_t lst[TW_WHEEL_SLOT_COUNT];
};

//时间轮
struct tw_t
{
    tw_wheel_t *wheels[TW_MAX_WHEEL_LEVEL]; //一个时间轮包含N个轮子
    dlinklist_t todo;   //待添加列表
    std::atomic_bool exited;    //
    pthread_t tid;
    std::unordered_map<uint64_t,tw_timer_t*> id_2_timer;
}g_tw;

uint64_t tw_max_ms(){
    uint64_t max_ms = 0;
    auto level = 1;
    while(level <= TW_MAX_WHEEL_LEVEL){
        max_ms+=std::pow(TW_WHEEL_SLOT_COUNT,level++);
    }
    return max_ms;
}
const static int64_t TW_MAX_MS = tw_max_ms();

uint64_t get_current_ms()  {
   struct timeval tv;  
   gettimeofday(&tv,NULL);  
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;  
}

uint64_t get_current_us(){
   struct timeval tv;  
   gettimeofday(&tv,NULL);  
   return tv.tv_sec * 1000 * 1000 + tv.tv_usec;  
}

void tw_add_to_wheel(tw_wheel_t *wheel,tw_timer_t* task){
    //uint64_t ms = task->ms;
    //根据轮子当前tick校正
    uint64_t revised_ms = wheel->tick + task->ms;

    //计算slot步长
    uint8_t steps = revised_ms / wheel->slot_size;
    //计算插入slot位置
    int8_t inert_idx = (steps + wheel->idx) % TW_WHEEL_SLOT_COUNT;
    //slot位置重叠修正
    uint64_t overflow_ms = 0;

    //槽内溢出，修正
    if(revised_ms >= wheel->slot_size){
        task->ms = revised_ms % wheel->slot_size;
    }

    //步长等于slot数，即当前位置，重叠修正
    if(steps == TW_WHEEL_SLOT_COUNT){
        overflow_ms = (wheel->slot_size - wheel->tick);
    }
    task->ms += overflow_ms;
    //printf("[%lu] add task4(%u), ms:%d, wheel->tick:%d, revised_ms:%d, overflow_ms:%lu, steps:%u, task->ms:%d,wheel->max:%d, wheel->idx:%d, idx:%d\n",get_current_us() ,task->id, ms,wheel->tick,revised_ms, overflow_ms,steps, task->ms ,wheel->max,wheel->idx,inert_idx);
    AddTail(&wheel->lst[inert_idx],task);
}

void tw_add_to_wheel(tw_timer_t* task){
    size_t level = 0;
    while(level < TW_MAX_WHEEL_LEVEL){
        tw_wheel_t *wheel = g_tw.wheels[level++];
        if(task->ms <= wheel->max ){
            tw_add_to_wheel(wheel,task);
            break;
        }else{
            task->ms-=wheel->max;
        }
    }
}

//异步添加计时器，会存在延时
void tw_add_to_wheel(){

    tw_timer_t* task = nullptr;
    do{
        task =  (tw_timer_t*)PoPHead(&g_tw.todo);
        if(task) {
            tw_add_to_wheel(task);
        }
    }while(task);
}

//添加计时器
uint64_t tw_add_timer(int64_t ms, tw_callback_func func, void *args){
    if(ms <= 0 || ms >= TW_MAX_MS){
        return -1;
    }

    tw_timer_t *task = new tw_timer_t;
    task->id = _global_id++;
    task->ms = ms;
    task->data = args;
    task->callback = func;
    g_tw.id_2_timer[task->id] = task;
    //异步添加，有延迟，最多1ms
    AddTail(&g_tw.todo,task);
    //同步添加
    //tw_add_to_wheel(task);
    return task->id;
}

/*
//删除计时器
*/
void tw_del_timer(uint64_t id){
    
    if (g_tw.id_2_timer.find(id)!=g_tw.id_2_timer.end()){
        tw_timer_t* task = g_tw.id_2_timer[id];
        RemoveFromLink(task);
        delete task;
        task = nullptr;
        g_tw.id_2_timer.erase(id);
    }
}

/*
//时间轮循化
*/
void tw_loop_wheels(){
    for(size_t level = 0; level < TW_MAX_WHEEL_LEVEL; level++)
    {
        tw_wheel_t *wheel = g_tw.wheels[level];
        dlinklist_t *lst = &wheel->lst[wheel->idx];
        tw_timer_t *head = (tw_timer_t *)PoPHead(lst);

        while(head!=nullptr && !g_tw.exited){
            //提升
            tw_timer_t *tmp = (tw_timer_t *)NextNode(head);
            //过期
            //是否需要降层
            if(level > 0){
                if(head->ms-- == 0){
                    //printf("[%lu] loop task(%d), level:%d, ms:%lu, round->idx:%d, round->tick:%d\n",get_current_ms(),head->id,level,head->ms,round->idx,round->tick);
                    RemoveFromLink(head);
                    //重新计算时间
                    head->ms = g_tw.wheels[level-1]->max;
                    tw_add_to_wheel(g_tw.wheels[level-1],head);
                }
            }else{
                if(head->ms-- == 0){
                    head->callback(head->id,head->data);
                    tw_del_timer(head->id);
                }
            }
            head = tmp;
        }

        //wheel tick
        if(++wheel->tick == wheel->slot_size){
            wheel->tick = 0;
            (++wheel->idx)%=TW_WHEEL_SLOT_COUNT;
        }
    }
}

void tw_sleep_us(int64_t us, bool _usl){
    if(us <= 0){
        return;
    }

    if(_usl){
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = us;
        int err = select(0,nullptr,nullptr,nullptr,&tv);
        if(err != 0){
            printf("timeout(%lu), error:%d\n",us,err);
        }
    }else{
        usleep(us);
    }
}

/*
//主循环
*/
void* tw_loop(void* args)
{
    uint64_t start,used = 0;
    while(!g_tw.exited){
        start =  get_current_us();
        tw_add_to_wheel();
        tw_loop_wheels();
        used = get_current_us() - start ;
        //小于1ms，休眠补足
        if(used < 1000 ){
            int64_t sleep_us = 1000 - used;
            int64_t s1 = get_current_us();
            tw_sleep_us(sleep_us,true);
            int64_t s2 = get_current_us();
            //printf("[%lu] tw_loop used: %lu, sleep:%lu, real:%lu\n",get_current_us(),used,sleep_us,s2-s1);
        }else{
            printf("tw_loop used: %lu\n",used);
        }
    }
    return nullptr;
}


/*
//初始化
*/
void tw_init(){    
    for(size_t level = 0; level < TW_MAX_WHEEL_LEVEL; level++)
    {
        if(g_tw.wheels[level] == nullptr){
            g_tw.wheels[level] = new tw_wheel_t;
            g_tw.wheels[level]->max = std::pow(TW_WHEEL_SLOT_COUNT,level+1);
            g_tw.wheels[level]->slot_size = g_tw.wheels[level]->max/TW_WHEEL_SLOT_COUNT;
        }
    }

    pthread_create(&g_tw.tid,nullptr,&tw_loop,nullptr);
    printf("tw_init done.\n");
}

/*
//回收
*/
void tw_destroy(){
    if(g_tw.exited)
    {
        g_tw.exited = true;
        pthread_join(g_tw.tid,nullptr);
        printf("tw_destroy done.\n");
    }
}