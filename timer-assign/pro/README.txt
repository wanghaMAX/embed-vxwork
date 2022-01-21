Get start.

Method1. Using shell.

1. rebuild all.
2. start vxWorks.exe (launch vxSim)
3. launch shell.
4. type 'task_marquee_start()' in shell, and you can see marquee in led.
5. type 'task_ddled_start()' in shell, and you can see led flashing with different frequency.

Method2. Motify source code.

1. open file Project2/usrAppInit.c
About marquee show.
2. uncomment the code 'task_marquee_start();'
3. rebuild all and start.
About leds flashing with different frequency.
4. uncomment the code 'task_ddled_start();'
5. rebuild all and start.
