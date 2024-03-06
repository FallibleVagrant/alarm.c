alarm: alarm.c
	gcc -o alarm alarm.c -Wall

release: alarm.c
	gcc -o alarm alarm.c -Wall -O4
