#ifndef M_MEM
#define M_MEM

#include <stdio.h>
#include <semLib.h>

void M_mem_init();
void M_mem_query();
void * M_mem_get(int size);
int  M_mem_put(void * blk);
void M_mem_destroy();

void init_func(void);
void task1_func(void);
void task2_func(void);
void task3_func(void);
void task4_func(void);
void task5_func(void);
void task_multi_blks_get_func(int *, int, int);
void task_multi_blks_put_func(int *, int);

#endif