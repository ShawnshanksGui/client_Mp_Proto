#include "unistd.h"

#include "../include/timer.h"
#include "../include/system_params.h"


#define START 1
#define END   0
#define STOP  0

#define YES   1

using namespace std;

Timer::Timer() {
	num_timeSlice = NUM_TIMESLICE;
	len_timeSlice = LEN_TIMESLICE;
}  

//notice whether ought to close the current thread or not 
extern int Terminal_AllThds;
extern int Terminal_RecvThds;
extern int Terminal_DecdThds;
extern int Terminal_WriterThds;

//==========================================================================
//==========================================================================
//Author:      shawnshanks_fei         Date:        20180305
//Description: the function which controls the tempo of all the processes 
//Parameter:   len_timeSlice denotes the lenth of one time slice
//             (of one video segment);
//             startFlag_one_timeSlice directly control the switch of all 
//             the processes; 
//==========================================================================
void Timer::setTimer_td_func() {
	usleep((int)(num_timeSlice * len_timeSlice * 1000000));
//	usleep(0.01* 1000000);
	Terminal_AllThds = YES;
//	usleep(50000);
//	Terminal_DecdThds = YES;
//	Terminal_WriterThds = YES;
}
//==========================================================================
