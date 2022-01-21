/* usrAppInit.c - stub application initialization routine */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,02jun98,ms   written
*/

/*
DESCRIPTION
Initialize user application code.
*/ 

/******************************************************************************
*
* usrAppInit - initialize the users application
*/ 

#include "M_mem.h"

int * blks[250];

void usrAppInit (void)
{
#ifdef	USER_APPL_INIT
	USER_APPL_INIT;		/* for backwards compatibility */
#endif

    /* add application specific code here */
	taskSpawn("M_mem_init",
		90,
		0x0100,
		2048*3,
		init_func,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void test1()
{
	taskSpawn("test1",
		92,
		0x0100,
		1024,
		task1_func,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void test2()
{
	taskSpawn("test2",
		91,
		0x0100,
		1024,
		task2_func,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void test3()
{
	taskSpawn("test3",
		93,
		0x0100,
		1024,
		task3_func,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void test4()
{
	taskSpawn("test4",
		93,
		0x0100,
		1024,
		task4_func,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void test5()
{
	taskSpawn("test5",
		93,
		0x0100,
		1024,
		task5_func,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void test6()
{
	int size = 20, i = 0, nums = 250;
	for(i=0; i<nums; i++){
		blks[i] = (void *)0;
	}

	taskSpawn("test61",
		93,
		0x0100,
		1024,
		task_multi_blks_get_func,
		blks, size, nums, 0, 0, 0, 0, 0, 0, 0);

	taskSpawn("test62",
		93,
		0x0100,
		1024,
		task_multi_blks_put_func,
		blks, nums, 0, 0, 0, 0, 0, 0, 0, 0);
}

void test7()
{
	int size = 10, i = 0, nums = 250;
	for(i=0; i<nums; i++){
		blks[i] = (void *)0;
	}

	taskSpawn("test71",
		93,
		0x0100,
		1024,
		task_multi_blks_get_func,
		blks, size, nums, 0, 0, 0, 0, 0, 0, 0);

	taskSpawn("test72",
		94,
		0x0100,
		1024,
		task_multi_blks_put_func,
		blks, nums, 0, 0, 0, 0, 0, 0, 0, 0);
}

void test8()
{
	int size = 20, i = 0, nums = 250;
	for(i=0; i<nums; i++){
		blks[i] = (void *)0;
	}

	taskSpawn("test81",
		94,
		0x0100,
		1024,
		task_multi_blks_get_func,
		blks, size, nums, 0, 0, 0, 0, 0, 0, 0);

	taskSpawn("test82",
		93,
		0x0100,
		1024,
		task_multi_blks_put_func,
		blks, nums, 0, 0, 0, 0, 0, 0, 0, 0);
}
