#include "unistd.h"

#include "../include/timer.h"
#include "../include/system_params.h"


#define START 1
#define STOP  0

#define YES   1

using namespace std;

Timer::Timer() {
	num_timeSlice = NUM_TIMESLICE;
	len_timeSlice = LEN_TIMESLICE;
}

//notice whether the client has already received the first packet or not  
extern int Flag_AlreadRecv;

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
//	while(num_timeSlice--) 
//	while(1) {
		while(!Flag_AlreadRecv);
		usleep(5 * len_timeSlice*1000000);
		Terminal_AllThds = YES;
//		startFlag_one_timeSlice = STOP;
//		usleep(len_timeSlice*1000000);		
//equal to YES
//	terminalFlag =  YES;
//	}
}
//==========================================================================
