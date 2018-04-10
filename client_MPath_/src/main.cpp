#include <chrono>
#include <string>
#include <vector>
#include <thread>

#include "fstream"
#include "sstream"
#include "iostream"
#include "ios"

#include "../include/timer.h"
#include "../include/common.h"
#include "../include/mySocket.h"
#include "../include/decoder.h"

#include "../include/data_manager.h"
#include "../include/video_writer.h"
#include "../include/system_params.h"

#include "../include/myUtility.h"

using namespace std;

//server to receiver
#define START 1
#define END   0
#define STOP  0

//client to server
#define REQUEST 1 

//notice whether ought to close the corresonding threads or not
//global external varible for threads' start and stop flag
int Terminal_AllThds = 0;
int Terminal_RecvThds = 0;
int Terminal_DecdThds = 0;
int Terminal_WriterThds = 0;


//two channels' realtime infomation
Channel_Inf chan_inf[NUM_PATH] = {{0.03, 50.0, 50.0}, {0.05, 90.0, 25.0}};
//Tile_Num tile_num{TILE_NUM, FOV_TILE_NUM, 
//	              CUSHION_TILE_NUM, OUTMOST_TILE_NUM};
int tile_num[REGION_NUM] = {FOV_TILE_NUM, CUSHION_TILE_NUM, 
							 OUTMOST_TILE_NUM};
//the unit is Mb/s
double _bitrate[BITRATE_TYPE_NUM] = {50.0, 25.0, 10.0};


int main(int argc, char **argv) {

	int Total_SegNum = 1;

//the control subflow	
	Transmitter client_ctrl;
	client_ctrl.transmitter_new(argv[9], argv[10], argv[11], argv[12]);

	while(Total_SegNum--) {
		char pkt_recv[10] = {'\0'};
		char pkt_ctrl[10] = {'\0'};

		pkt_ctrl[0] = REQUEST;
		client_ctrl.Send_tcp(pkt_ctrl, 10);
		client_ctrl.Recv_tcp(pkt_recv, 10);

		if(pkt_recv[0] == START) {
//inform all the threads of starting
			Terminal_AllThds = NO;
//==========================================================================
//class arguments!!!
			Timer t;
			Data_Manager data_manager;
			vector<Decoder> decoder(NUM_PATH);
//the two data subflows!!!
			vector<Transmitter> client_data(NUM_PATH);
			vector<Video_Writer> video_writer(NUM_PATH);
//thread(class) arguments!!!
			vector<thread> decoder_worker(NUM_PATH);
			vector<thread> receiver_worker(NUM_PATH);
			vector<thread> writer_worker(NUM_PATH);	
//===========================================================================

//the member function!!!
			client_data[0].transmitter_new_tcp_non_b_sponsor(argv[1], argv[2],
															 argv[3], argv[4]);
			client_data[1].transmitter_new_tcp_non_b_sponsor(argv[5], argv[6], 
															 argv[7], argv[8]);

//===========================================================================
//create corresponding threads!!!
			thread setTimer_worker(&Timer::setTimer_td_func, &t);
//i specifies the id of path, 
//i+2 identifies the id of core bind to the thread
			for(int i = 0; i < NUM_PATH; i++) {			
				receiver_worker[i] = thread(&Transmitter::recv_td_func, 
					                	    &(client_data[i]),i,
					                	    i,ref(data_manager));
				decoder_worker[i]  = thread(&Decoder::decoder_td_func,
								 		    &(decoder[i]), i+2, i,
								 		    ref(data_manager));
				writer_worker[i]   = thread(&Video_Writer::video_writer_td_func,
										    &(video_writer[i]), i,
										    ref(data_manager));
			}
//===========================================================================

//reap or recycle the threads created.	
			for(int i = 0; i < NUM_PATH; i++) {
				receiver_worker[i].join();
				decoder_worker[i].join();
				writer_worker[i].join();
			}
		}
		else if(pkt_recv[0] == END){
			break;
		}	
	}	
//	setTimer_worker.join();
	return 0;
}	


/*
int main(int argc, char **argv) {
	char packet[1000] = {'\0'};
	int len = 1000;

	long int cnt_byte = 0;
	int cnt_pkt = 0;

	Transmitter client;
	
	client.transmitter_new_tcp_sponsor(argv[1], argv[2], argv[3], argv[4]);
//	client.transmitter_new_tcp_sponsor(argv[5], argv[6], argv[7], argv[8]);

	auto startTime = std::chrono::high_resolution_clock::now();
	
	while(1) {
		memset(packet, 0, len);

//		cnt_byte += client.Recv_tcp(packet, len);
	    cnt_byte += client.Recv_tcp_fixed_len(packet, len);

		if(packet[0] == 0) {
			break;
		}
		
//		client.Send_tcp(packet, len);
		cnt_pkt++;		
//		printf("this is the %d-th pkt\n", ++cnt_pkt);
	}
	printf("\nleave the while recyle, received %d packets, and %ld bytes data !\n", \
		   cnt_pkt, cnt_byte);
   	
	auto endTime  = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
	printf("\nthe duration is %d us\n", duration);
	
	return 0;
}
*/