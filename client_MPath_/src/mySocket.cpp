#include "../include/thread_core_affinity_set.h"

#include <memory>
#include <queue>

#include "../include/system_params.h"
#include "../include/encoder.h"
#include "../include/common.h"
#include "../include/mySocket.h"

#include "../include/data_manager.h"

#define ON_REUSEADDR  1  //you can reuse the addr after binding addr without no waiting time 
//#define OFF_REUSEADDR 0

#define DATA 1
#define CONTROL 2

#define INIT_VAL 255

//global variable flag for noticing whether already received the first packet
int Flag_AlreadRecv = 0;

//notice whether ought to close the current thread or not
//global extrenal variabl for terminal flags
extern int Terminal_AllThds;
extern int Terminal_RecvThds;


Transmitter::~Transmitter() {
	if(sock_id > 0)
		close(sock_id);
}

void Transmitter::Socket_for_udp() {
	if((sock_id = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Create socket falied\n");
		exit(0);
	}
}

void Transmitter::Setsockopt(int sock_id, int level, int option_name,
               				 void *option_value, int option_len) {
    if(-1 == setsockopt(sock_id, level, 
    	option_name, option_value, option_len)) {
        perror("setsockopt failed!\n");
        exit(0);
    }
}

void Transmitter::Bind(int sock_id, SA *addr_self, int len) const 
{
	if(bind(sock_id, addr_self, len)) {
		perror("bind failed!!!");
		exit(0);
	}
}


void Transmitter::
transmitter_new(char *addr_self, char *port_self, 
				char *addr_dst, char *port_dst) {
	memset(&(server_addr), 0, sizeof(server_addr));
	memset(&(client_addr), 0, sizeof(client_addr));
	
	Socket_for_udp();

//enable fast recovering the port which have being used. 
	int state_reuseAddr              = ON_REUSEADDR;
//	
	Setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, 
		       &state_reuseAddr, sizeof(state_reuseAddr));

	server_addr.sin_family  = AF_INET;
	server_addr.sin_port    = htons(atoi(port_dst));
	inet_pton(AF_INET, addr_dst, &(server_addr.sin_addr));

	client_addr.sin_family = AF_INET;
	client_addr.sin_port   = htons(atoi(port_self));
	inet_pton(AF_INET, addr_self, &(client_addr.sin_addr));

	Bind(sock_id, (struct sockaddr *)&client_addr, sizeof(client_addr));
//  udp needn't establish any connection!!!
//	Connect(sock_id, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));  
}

int Transmitter::
Send_udp(char *data, int len) {
	int num_sent = 0;

	if((num_sent = sendto(sock_id, data, len, 0, 
		(SA *)&(server_addr), sizeof(SA))) < 0) {
		printf("\n!!!send failed, send %d bytes!!!!!!\n", num_sent);
		exit(0);
	}
	return num_sent;
}

int Transmitter::
Recv_udp(char *buf_dst, int len) {
	int num_recv = 0;
	socklen_t len_server_addr;

	if ((num_recv = recvfrom(sock_id, buf_dst, len, 0, 
	   (SA *)&(server_addr), &len_server_addr)) < 0) {
		printf("\n!!!recv failed, just recv %d bytes!!!!!!\n", num_recv);
		exit(0);
	}
	return num_recv;
}


void Transmitter::
recv_td_func(int id_core, int id_path, Data_Manager &data_manager) {

	affinity_set(id_core);

	int cnt_pkt    = 0;
	int cnt_symbol = 0;
	
//	uchar prev_seg_id    = INIT_VAL;
	uchar prev_block_id  = INIT_VAL;
//	uchar symbol_id_prev = INIT_VAL;

//in case of the error of no decalration
	shared_ptr<struct Block_Data> block_data;

	uchar id_seg = 0; uchar id_region = 0; uchar block_id = 0; 
	uchar symbol_id = 0; uchar s_level = 0; uchar k_fec= 0; uchar m_fec = 0;
	int originBlk_size = 0;;

	VData_Type packet[SYMBOL_LEN_FEC + LEN_CONTRL_MSG];

	printf("\nenter the while recycle of recv_thread\n");
	while(!Terminal_AllThds && !Terminal_RecvThds) {

		memset(packet, 0, SYMBOL_LEN_FEC + LEN_CONTRL_MSG);
		
		printf("wait for receiving!\n");
		Recv_udp(packet, SYMBOL_LEN_FEC + LEN_CONTRL_MSG);
		printf("*received the %d-th packet*\n", ++cnt_pkt);
//notice the setTimer thread to start the timer
		if(Flag_AlreadRecv != YES) {Flag_AlreadRecv = YES;}

		if(DATA == packet[0]) {
			decaps_pkt(packet, id_seg, id_region, block_id, symbol_id,
					   originBlk_size, s_level, k_fec, m_fec);
//			if(id_seg == prev_seg_id) {
				if(block_id != prev_block_id) {
//psuh block_data into recvQ_data[id_path], if not the first block 
					if(INIT_VAL != prev_block_id) {
						block_data->cnt_s = cnt_symbol;
						data_manager.recvQ_data[id_path].push(block_data);
						cnt_symbol = 0;
					}
					prev_block_id = block_id;
// initialize the block data ,preparing for the new block
					shared_ptr<struct Block_Data> block_data = \
					(shared_ptr<struct Block_Data>)new(struct Block_Data);

					block_data->data = MALLOC(VData_Type *, 256);
					block_data->erasure = MALLOC(int, k_fec+m_fec);

					for(int i = 0; i < k_fec + m_fec; i++) {
						block_data->erasure[i] = LOST;
					}
					
					if(s_level == 0) {block_data->S_FEC = SYMBOL_LEN_FEC;}
					else {printf("s_level = %d,  fault happened!!!\n", s_level);}

					block_data->K_FEC = k_fec;
					block_data->M_FEC = m_fec;
					block_data->originBlk_size = originBlk_size;
					block_data->id_seg = id_seg;
					block_data->id_region = id_region;

					VData_Type *symbol = MALLOC(VData_Type,block_data->S_FEC);
					memcpy(symbol, &(packet[LEN_CONTRL_MSG]), block_data->S_FEC);
					block_data->data[cnt_symbol] = symbol;

					cnt_symbol++;
					block_data->erasure[symbol_id] = GET;
				}

				else {
					VData_Type *symbol = MALLOC(VData_Type,block_data->S_FEC);
					memcpy(symbol, &(packet[LEN_CONTRL_MSG]), block_data->S_FEC);
					block_data->data[cnt_symbol] = symbol;

					cnt_symbol++;
					block_data->erasure[symbol_id] = GET;
				}

//			}
		}
//fetch the data from send_Q, queue buffer

	}
}

//==========================================================================
//==========================================================================
//Author:      shawnshanks_fei          Date:  
//Description: implement packet decapsualtion procedure 
//Parameter:   size denotes how much data the data block in real apart 
//             from the number of 0 padding.  
//==========================================================================
void Transmitter::
decaps_pkt(VData_Type *packet, uchar &id_seg, uchar &id_region, 
		   uchar &block_id, uchar &symbol_id, int &originBlk_size,
		   uchar &s_level, uchar &k_fec, uchar &m_fec) {
	id_seg    = (uchar)packet[1];
	id_region = (uchar)packet[2];

	block_id  = (uchar)packet[7];
	symbol_id = (uchar)packet[8];
	s_level   = (uchar)packet[9];
	k_fec     = (uchar)packet[10];
	m_fec     = (uchar)packet[11];
	//......
	
	originBlk_size  = *((int *)&(packet[3]));
//for debugging
	printf("the packet info is:\n");
	printf("id_seg = %d, id_region = %d, originBlk_size = %d, block_id = %d, symbol_id = %d, \
		s_level = %d, k_fec = %d, m_fec = %d\n", id_seg, id_region, originBlk_size, \
		block_id, symbol_id, s_level, k_fec, m_fec);
}
/*
void Transmitter::encaps_packet(VData_Type *packet, int symbol_id, VData_Type *data_src, 
								shared_ptr <struct Elem_Data> data_elem) {
//specifies the type of packet	
	packet[0] = DATA;
	packet[1] = data_elem->id_path;
	packet[2] = data_elem->id_seg;
 	packet[3] = data_elem->size;
	packet[4] = data_elem->type_nalu;
	packet[5] = block_id;
	packet[6] = symbol_id;
	packet[7] = data_elem->K_FEC;
//specify which one S in the current block. 


	memcpy(&(packet[9]), data_src, data_elem->S_FEC);	
}
*/


/*
//udp protocol doesn't neede to establish any connections!!!
void Transmitter::Connect(int sock_id, struct sockaddr *serv_addr, int len_sock_addr) const {
	if(-1 == connect(sock_id, serv_addr, len_sock_addr)) {
       	perror("Connect socket failed!\n");
        exit(0);
}
}
*/

/*
void Transmitter::transmitter_new(char *addr_self, char *port_self, 
	                              char *addr_dst, char *port_dst) {
	memset(&(server_addr), 0, sizeof(server_addr));
	memset(&(client_addr), 0, sizeof(client_addr));
	
	Socket_for_udp();

//enable fastly recover the port which have being used. 
	int state_reuseAddr              = ON_REUSEADDR;
//	
	Setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, 
		       &state_reuseAddr, sizeof(state_reuseAddr));

	server_addr.sin_family  = AF_INET;
	server_addr.sin_port    = htons(atoi(port_dst));
	inet_pton(AF_INET, addr_dst, &(server_addr.sin_addr));

	client_addr.sin_family = AF_INET;
	client_addr.sin_port   = htons(atoi(port_self));
	inet_pton(AF_INET, addr_self, &(client_addr.sin_addr));

	Bind(sock_id, (struct sockaddr *)&client_addr, sizeof(client_addr));
//  udp needn't establish any connection!!!
//	Connect(sock_id, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));  
}
*/
