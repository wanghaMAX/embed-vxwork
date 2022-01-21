#include <stdio.h>
#include <semLib.h>
#include "M_mem.h"

#define POOL1_SIZE  128
#define POOL2_SIZE  64
#define BLOCK1_SIZE 16
#define BLOCK2_SIZE 32

typedef struct M_mem_partition{
	void * pool1_start_ptr;
	void * pool2_start_ptr;
	void * pool1;
	void * pool2;
	int    pool1_cap;
	int    pool2_cap;
	int    pool1_used;
	int    pool2_used;
	int    pool1_blk_size;
	int    pool2_blk_size;
	SEM_ID mutex;
}M_part_struct;

static M_part_struct part;

/*
 * pool_status value meaning
 * 0: system_heap 1:pool1 2:pool2
 */
typedef int pool_status;

void M_mem_init()
{
	int i = 0;
	char * p;
	void ** plink;

	part.pool1 = (void *)malloc(POOL1_SIZE * BLOCK1_SIZE);
	part.pool2 = (void *)malloc(POOL2_SIZE * BLOCK2_SIZE);
	if (part.pool1 == NULL || part.pool2 == NULL){
		printf("INIT ERROR");
		free(part.pool1);
		free(part.pool2);
	}
	part.pool1_start_ptr = part.pool1;
	part.pool2_start_ptr = part.pool2;

	part.pool1_cap = POOL1_SIZE;
	part.pool2_cap = POOL2_SIZE;
	part.pool1_used = 0;
	part.pool2_used = 0;
	part.pool1_blk_size = BLOCK1_SIZE;
	part.pool2_blk_size = BLOCK2_SIZE;

	p = part.pool1;
	plink = (void *)part.pool1;
	*plink = p;
	
	for(i=0; i<POOL1_SIZE-1; i++){
		p += part.pool1_blk_size;
		plink = (void *)p;
		*plink = (void **)p;
	}
	*plink = NULL;

	p = part.pool2;
	plink = (void *)part.pool2;
	*plink = p;

	for(i=0; i<POOL2_SIZE-1; i++){
		p += part.pool2_blk_size;
		plink = (void *)p;
		*plink = (void **)p;
	}
	*plink = NULL;

	part.mutex = semMCreate(0x4); /* delete safe & FIFO*/
}

void * M_mem_get(int size)
{
	void * p;
	semTake(part.mutex, WAIT_FOREVER);
	if(size < part.pool1_blk_size && part.pool1_used < part.pool1_cap){
		p = part.pool1;
		part.pool1 = *(void **)p;
		part.pool1_used ++;
		semGive(part.mutex);
		return p;
	}
	if(size < part.pool2_blk_size && part.pool2_used < part.pool2_cap){
		p = part.pool2;
		part.pool2 = *(void **)p;
		part.pool2_used ++;
		semGive(part.mutex);
		return p;
	}
	p = malloc(size);
	semGive(part.mutex);
	return p;
}

int M_mem_put(void * blk)
{
	int res = 0;
	semTake(part.mutex, WAIT_FOREVER);
	if((int)(blk - part.pool1_start_ptr) < part.pool1_cap * part.pool1_blk_size &&
	   (int)(blk - part.pool1_start_ptr) >= 0){
		*(void **)blk = part.pool1;
		part.pool1 = blk;
		part.pool1_used --;
	}else if((int)(blk - part.pool2_start_ptr) < part.pool2_cap * part.pool2_blk_size &&
	         (int)(blk - part.pool2_start_ptr) >= 0){
		*(void **)blk = part.pool2;
		part.pool2 = blk;
		part.pool2_used --;
	}else{
		free(blk);
	}
	semGive(part.mutex);
	return res;
}

void M_mem_destroy()
{
	free(part.pool1_start_ptr);
	free(part.pool2_start_ptr);
}

void M_mem_query()
{
	printf( "\nM_mem_status\n"
		"Pool1:\n"
		"Usage: %d/%d\n"
		"Pool2:\n"
		"Usage: %d/%d\n",
		part.pool1_used, part.pool1_cap,
		part.pool2_used, part.pool2_cap);
}


void init_func(void)
{
	printf("INIT MY MEMORY PARTION\n\n");
	M_mem_init();
}

void task1_func()
{
	int* arr[40];
	int size1 = 5, size2 = 20;
	int i=0;

	printf("=== TEST 1 ===");

	/* M_mem_init(); */
	M_mem_query();

	for(i=0; i<20; i++){
		arr[i] = M_mem_get(size1);
	}
	M_mem_query();

	for(i=20; i<40; i++){
		arr[i] = M_mem_get(size2);
	}
	M_mem_query();

	for(i=0; i<40; i++){
		M_mem_put(arr[i]);
	}
	M_mem_query();
	exit();
}

void task2_func()
{
	int* arr[129];
	int size1 = 5;
	int i=0;

	printf("=== TEST 2 ===");

	/* M_mem_init();*/
	M_mem_query();

	for(i=0; i<129; i++){
		arr[i] = M_mem_get(size1);
	}
	M_mem_query();

	for(i=0; i<129; i++){
		M_mem_put(arr[i]);
	}
	M_mem_query();
	exit();
}

void task3_func()
{
	int* arr[193];
	int size1 = 5;
	int i=0;

	printf("=== TEST 3 ===");

	/* M_mem_init();*/
	M_mem_query();

	for(i=0; i<193; i++){
		arr[i] = M_mem_get(size1);
	}
	M_mem_query();

	for(i=0; i<193; i++){
		M_mem_put(arr[i]);
	}
	M_mem_query();
	exit();
}

void task4_func()
{
	int* arr[65];
	int size1 = 20;
	int i=0;

	printf("=== TEST 4 ===");

	/* M_mem_init();*/
	M_mem_query();

	for(i=0; i<65; i++){
		arr[i] = M_mem_get(size1);
	}
	M_mem_query();

	for(i=0; i<65; i++){
		M_mem_put(arr[i]);
	}
	M_mem_query();
	exit();
}

void task5_func()
{
	int* arr[5];
	int size1 = 80;
	int i=0;

	printf("=== TEST 5 ===");

	/* M_mem_init();*/
	M_mem_query();

	for(i=0; i<5; i++){
		arr[i] = M_mem_get(size1);
	}
	M_mem_query();

	for(i=0; i<5; i++){
		M_mem_put(arr[i]);
	}
	M_mem_query();
	exit();
}

void task_multi_blks_get_func(int * blks, int size, int nums)
{
	int i=0;
	for(i=0; i<nums; i++){
		blks[i] = M_mem_get(size);
	}
	M_mem_query();
	exit();
}

void task_multi_blks_put_func(int * blks, int nums)
{
	int i=0;
	for(i=0; i<nums; i++){
		while(blks[i] == (void *)0);
			M_mem_put(blks[i]);
	}
	M_mem_query();
	exit();
}
