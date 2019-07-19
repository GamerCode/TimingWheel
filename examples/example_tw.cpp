#include "tw.h"
#include <unistd.h>
#include <iostream>
using std::cout;
using std::cin;
using std::endl;

uint32_t prev_id = 0;
const int max_id = 3000;

void test(uint32_t id, void* data){
    //if (prev_id != id-1 || max_id == id){
        cout<<"["<<get_current_ms()<<"] id:"<<prev_id<<" -> id:"<<id<<endl;
    //}
    prev_id = id;
}

int main(int argc, char const *argv[])
{
    tw_init();
    for(size_t i = 1; i <= max_id; i++)
    {
        tw_add_timer(i,&test,nullptr);
    }

    while(prev_id != max_id){
        tw_sleep_us(1000,false);
    }
    tw_destroy();
    return 0;
}