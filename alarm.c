/* alarm - delay until a specific time. */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define DEBUG 0

#define dbgprint(...) \
            do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

//The sleep period acts like a sliding window.
#define MAX_SLEEP_PERIOD		60
#define SLEEP_INIT				0.5
double sleep_period = SLEEP_INIT;

struct timespec alarm_time;
struct timespec previous_time;
struct timespec current_time;

//Returns the difference between x and y; order DOES matter.
struct timespec get_remaining_time(struct timespec alarm_time, struct timespec current_time){
	struct timespec result = {0};

	if(alarm_time.tv_nsec > current_time.tv_nsec){
		result.tv_nsec = alarm_time.tv_nsec - current_time.tv_nsec;
	}
	else if(alarm_time.tv_nsec <= current_time.tv_nsec){
		result.tv_nsec = 0;
	}

	if(alarm_time.tv_sec > current_time.tv_sec){
		result.tv_sec = alarm_time.tv_sec - current_time.tv_sec;
	}
	else if(alarm_time.tv_sec == current_time.tv_sec){
		result.tv_sec = 0;
	}
	//alarm_time.tv_sec < current_time.tv_sec
	else{
		result.tv_sec = 0;
		result.tv_nsec = 0;
	}
	
	return result;
}

void sleep_for_a_time(struct timespec previous_time, struct timespec current_time, struct timespec rem){
	//TODO: figure out why this function is optimized out unless there is a sleep function.
	printf("sleeping...\n");
	//Test if the system was suspended while this process was asleep.
	if(current_time.tv_sec - previous_time.tv_sec > sleep_period + 1){
		dbgprint("System was detected asleep, resetting sleep timer.\n");
		sleep_period = SLEEP_INIT;
	}
	//Calculate sleep_period.
	sleep_period *= 2;

	if(sleep_period > MAX_SLEEP_PERIOD){
		dbgprint("Hit above %d secs sleep period, setting to %d.\n", MAX_SLEEP_PERIOD, MAX_SLEEP_PERIOD);
		sleep_period = MAX_SLEEP_PERIOD;
	}
	//Test if alarm_time is close to current_time.
	if((long) sleep_period >= rem.tv_sec){
		//
		if(rem.tv_sec >= 1){
			dbgprint("sleep_period is close to rem, setting to %lf.\n", (double) rem.tv_sec / 2);
			sleep_period = (double) rem.tv_sec / 2;
		}
		//Sub-second time to wait, just spin.
		if(rem.tv_sec == 0){
			dbgprint("sleep_period is close to rem, setting to 0.\n");
			sleep_period = 0;
		}
	}

	dbgprint("Sleeping for time %lf secs.\n", sleep_period);
	//sleep(sleep_period);
	int res;
	struct timespec ts;
	ts.tv_sec = (long) sleep_period;
	do{
		res = nanosleep(&ts, &ts);
	}while(res && errno == EINTR);
}

double parse_to_seconds(const char* time_suffixed){
	dbgprint("parsing: %s\n", time_suffixed);
	char* endptr;
	double output = strtod(time_suffixed, &endptr);

	if(output == 0 && time_suffixed == endptr){
		return 0;
	}
	if(errno == ERANGE){
		return 0;
	}
	if(endptr == NULL){
		return output;
	}

	switch(*endptr){
		case 's':
			break;
		case 'm':
			output *= 60;
			break;
		case 'h':
			output *= 60 * 60;
			break;
		case 'd':
			output *= 60 * 60 * 24;
			break;
		default:
			return 0;
	}

	return output;
}

int main(int argc, char* argv[]){
	if(argc < 2){
		dbgprint("not enough arguments.\n");
		return -1;
	}
	double time_to_wait = 0;
	for(int i = 1; i < argc; i++){
		time_to_wait += parse_to_seconds(argv[i]);
	}
	dbgprint("Time to wait was inputted as: %lf.\n", time_to_wait);

	timespec_get(&alarm_time, TIME_UTC);
	alarm_time.tv_sec += time_to_wait;
	timespec_get(&current_time, TIME_UTC);
	dbgprint("Alarm time is: %ld.\n", alarm_time.tv_sec);
	dbgprint("Current time is: %ld.\n", current_time.tv_sec);

	struct timespec rem = get_remaining_time(alarm_time, current_time);
	while(rem.tv_sec > 0 || rem.tv_nsec > 0){
		previous_time = current_time;
		timespec_get(&current_time, TIME_UTC);

		rem = get_remaining_time(alarm_time, current_time);
		dbgprint("---------------------------Time remaining is: %ld sec %ld nsec.\n", rem.tv_sec, rem.tv_nsec);
		sleep_for_a_time(previous_time, current_time, rem);
	}

	return 0;
}
