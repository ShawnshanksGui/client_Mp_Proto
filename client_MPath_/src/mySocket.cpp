#include "../include/thread_core_affinity_set.h"

#include <memory>
#include <queue>

#include "poll.h"
#include "../include/common.h"

#include "../include/mySocket.h"
#include "../include/data_manager.h"

#include "../include/system_params.h"


#define PRINT_PROCEDURE(name) printf("\n%s\n", name)


#define ON_REUSEADDR  1  //you can reuse the addr after binding addr without no waiting time 
//#define OFF_REUSEADDR 0

#define DATA_PKT 1
#define CTRL_PKT 2

#define STOP_PKT 0


#define INIT_VAL 254

//global variable flag for noticing whether already received the first packet
//int Flag_AlreadRecv = 0;

//notice whether ought to close the current thread or not
//global extrenal variabl for terminal flags
extern int Terminal_AllThds;
extern int Terminal_RecvThds;


Transmitter::
~Transmitter() {
	if(sock_id > 0)
		close(sock_id);
}

void Transmitter::
Socket_for_udp() {
	if((sock_id = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Create socket falied\n");
		exit(0);
	}
}

void Transmitter::
Socket_for_tcp() {
	if((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create socket falied\n");
		exit(0);
	}	
}

void Transmitter::
Setsockopt(int sock_id, int level, int option_name,
           void *option_value, int option_len) {
    if(-1 == setsockopt(sock_id, level, 
    	option_name, option_value, option_len)) {
        perror("setsockopt failed!\n");
        exit(0);
    }
}

void Transmitter::
Bind(int sock_id, SA *addr_self, int len) const 
{
	if(-1 == bind(sock_id, addr_self, len)) {
		perror("bind failed!!!");
		exit(0);
	}
}

//  tcp needs to establish a connections!!!
void Transmitter::
Connect() {
	if(connect(sock_id, (SA *)&server_addr, sizeof(server_addr)) < 0) {
    	if(errno != EINPROGRESS) { 
	  		perror("Connect socket failed!\n");
        	exit(0);
		}
		else{
			perror("connect");
			printf("check delay connection\n");
			CheckConnect(sock_id);			
		}	
 	}
}


void Transmitter::
Connect_non_b() {
		int nsec = 10;
        int flags, n, error, code;  
        socklen_t len;  
        fd_set wset;  
        struct timeval tval;  
      
        flags = fcntl(sock_id, F_GETFL, 0);  
        fcntl(sock_id, F_SETFL, flags | O_NONBLOCK);  
      
        error = 0;  
        if ((n == connect(sock_id, (SA *)&server_addr, sizeof(server_addr))) == 0) {  
            goto done;  
        } else if (n < 0 && errno != EINPROGRESS){  
	  		perror("Connect socket failed!\n");
        	exit(0);  
        }  
      
        /* Do whatever we want while the connect is taking place */  
      
        FD_ZERO(&wset);  
        FD_SET(sock_id, &wset);  
        tval.tv_sec = nsec;  
        tval.tv_usec = 0;  
      
        if ((n = select(sock_id+1, NULL, &wset,   
                        NULL, nsec ? &tval : NULL)) == 0) {  
            close(sock_id);  /* timeout */  
	  		perror("Connect socket failed, timeout!\n");
        	exit(0);
        }  
      
        if (FD_ISSET(sock_id, &wset)) {  
            len = sizeof(error);  
            code = getsockopt(sock_id, SOL_SOCKET, SO_ERROR, &error, &len);  
     
            if (code < 0 || error) {  
                close(sock_id);  
                if (error)   
                    errno = error;  
	  			perror("Connect socket failed!\n");
        		exit(0);
            }  
        }
        else {  
            fprintf(stderr, "select error: sock_id not set");  
            exit(0);  
        }  
      
    done:  
        fcntl(sock_id, F_SETFL, flags);  /* restore file status flags */   
}

int Transmitter::
CheckConnect(int iSocket) {
	struct pollfd fd;
	int ret = 0;
	socklen_t len = 0;

	fd.fd = iSocket;
	fd.events = POLLOUT;

	while ( poll (&fd, 1, -1) == -1 ) {
		if( errno != EINTR ){
			perror("poll");
//			return -1;
			exit(0);
		}
	}

	len = sizeof(ret);
	if ( getsockopt (iSocket, SOL_SOCKET, SO_ERROR, &ret, &len) == -1 ) {
    	perror("getsockopt");
//		return -1;
		exit(0);
	}

	if(ret != 0) {
		fprintf (stderr, "socket %d connect failed: %s\n",
                 iSocket, strerror (ret));
//		return -1;
		exit(0);
	}

	return 0;
}

void Transmitter::
transmitter_new(char *addr_self, char *port_self, 
				char *addr_dst, char *port_dst) {
	memset(&(server_addr), 0, sizeof(server_addr));
	memset(&(client_addr), 0, sizeof(client_addr));
	
	Socket_for_udp();

//enable fastly recover the port which just has been occupied. 
	int state_reuseAddr              = ON_REUSEADDR;
	Setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, 
			   &state_reuseAddr, sizeof(state_reuseAddr));
//set the size of recv buffer
	int recv_buf_size=1*1024*1024*1024;
	Setsockopt(sock_id, SOL_SOCKET, SO_RCVBUF, (char *)&recv_buf_size, sizeof(int));

	server_addr.sin_family  = AF_INET;
	server_addr.sin_port    = htons(atoi(port_dst));
	inet_pton(AF_INET, addr_dst, &(server_addr.sin_addr));

	client_addr.sin_family = AF_INET;
	client_addr.sin_port   = htons(atoi(port_self));
	inet_pton(AF_INET, addr_self, &(client_addr.sin_addr));

	Bind(sock_id, (SA *)&client_addr, sizeof(client_addr));
//  udp needn't establish any connection!!!
//	Connect(sock_id, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));  
}

//defult blocking model
void Transmitter::
transmitter_new_tcp_sponsor(char *addr_self, char *port_self,
                			char *addr_dst, char *port_dst) {
    memset(&(server_addr), 0, sizeof(server_addr));
    memset(&(client_addr), 0, sizeof(client_addr));

    Socket_for_tcp();

//enable fastly recover the port which just has been occupied. 
    int state_reuseAddr              = ON_REUSEADDR;
    Setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR,
               &state_reuseAddr, sizeof(state_reuseAddr));
//set the size of recv buffer
    int recv_buf_size=1*1024*1024*1024;
    Setsockopt(sock_id, SOL_SOCKET, SO_RCVBUF, (char *)&recv_buf_size, sizeof(int));

    server_addr.sin_family  = AF_INET;
    server_addr.sin_port    = htons(atoi(port_dst));
    inet_pton(AF_INET, addr_dst, &(server_addr.sin_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_port   = htons(atoi(port_self));
    inet_pton(AF_INET, addr_self, &(client_addr.sin_addr));

    Bind(sock_id, (struct sockaddr *)&client_addr, sizeof(client_addr));

//  tcp need to establish a connection!!!
  	Connect();  
}

//set non-blocking mode
void Transmitter::
transmitter_new_tcp_non_b_sponsor(char *addr_self, char *port_self,
                			char *addr_dst, char *port_dst) {
    memset(&(server_addr), 0, sizeof(server_addr));
    memset(&(client_addr), 0, sizeof(client_addr));

    Socket_for_tcp();

//set non-blocking mode
//    int flags = fcntl(sock_id, F_GETFL, 0);
//    fcntl(sock_id, F_SETFL, flags|O_NONBLOCK);

//enable fastly recover the port which just has been occupied. 
    int state_reuseAddr              = ON_REUSEADDR;
    Setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR,
               &state_reuseAddr, sizeof(state_reuseAddr));
//set the size of recv buffer
    int recv_buf_size=1*1024*1024*1024;
    Setsockopt(sock_id, SOL_SOCKET, SO_RCVBUF, (char *)&recv_buf_size, sizeof(int));

    server_addr.sin_family  = AF_INET;
    server_addr.sin_port    = htons(atoi(port_dst));
    inet_pton(AF_INET, addr_dst, &(server_addr.sin_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_port   = htons(atoi(port_self));
    inet_pton(AF_INET, addr_self, &(client_addr.sin_addr));

    Bind(sock_id, (struct sockaddr *)&client_addr, sizeof(client_addr));

//  tcp need to establish a connection!!!
  	Connect_non_b();  
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
//		printf("\n!!!recv failed, just recv %d bytes!!!\n", num_recv);
//		exit(0);
	}
	return num_recv;
}


//==========================================================================
int Transmitter::
Send_tcp(char *data, int len) {
	int len_sent = 0;
	len_sent = send(sock_id, data, len, 0);
	if(-1 == len_sent) {
		perror("send failed");
		exit(0);
	}
	else if(len_sent != len) {
		printf("\nOnly send %dbytes data\n", len_sent);
	}
	return len_sent;
}

//default blocking mode
int Transmitter::
Recv_tcp(char *data_dst, int len) {
	int len_recv;
	len_recv = recv(sock_id, data_dst, len, 0);
	if(-1 == len_recv) {
		perror("\nrecv failed\n");
		exit(0);
	}
	else if(len_recv != len) {
//		printf("\nOnly recv %dbytes data\n", len_recv);
//		exit(0);
	}
	return len_recv;
}

//set non_blocking mode
int Transmitter::
Recv_tcp_non_b(char *data_dst, int len) {
//	int len_recv;
//	len_recv = recv(sock_id, data_dst, len, MSG_DONTWAIT);
	int tmp = 0;

	while(1) {
        if(Terminal_AllThds || Terminal_RecvThds) {
            break;
        }

//		printf("start enter the recv func\n");
//		tmp = recv(sock_id, data_dst, len, MSG_DONTWAIT);
		tmp = recv(sock_id, data_dst, len, MSG_DONTWAIT);
//		printf("leave the recv func\n");

		if(tmp < 0) {
			if(errno == EAGAIN) {
				usleep(1000);
				continue;
			}
			else {
				perror("receive failed in Recv_tcp_non_b!\n");
				exit(0);
			}
		}
		else {
			return tmp;
		}
	}

	return tmp;
}

int Transmitter::
Recv_tcp_fixed_len(char *data_dst, int len) {
    int tmp = 0;
    char *loc = data_dst;
    int len_specified = len;

    while(1) {
    	if(Terminal_AllThds || Terminal_RecvThds) {
    		break;
    	}
        tmp = Recv_tcp(loc, len_specified);
        if(tmp < len_specified) {
            len_specified -= tmp;
            loc += tmp;
        }
        else {break;}
    }

    return len;
}

int Transmitter::
Recv_tcp_non_b_fixed_len(char *data_dst, int len) {
    int tmp = 0;
    char *loc = data_dst;
    int len_specified = len;

//    printf("\n\n");
//    printf("already enter once the Recv_tcp_non_b_fixed_len\n");
    while(1) {
//		printf("already enter the Recv_tcp_non_b_fixed_len while\n");
    	if(Terminal_AllThds || Terminal_RecvThds) {
    		break;
    	}
//    	printf("%d ", len_specified);
//		printf("start entering  Recv_tcp_non_b\n");
    	tmp = 0;
//        tmp = recv(sock_id, loc, len_specified, MSG_DONTWAIT);
		tmp = recv(sock_id, loc, len_specified, 0);
//		printf("leave the Recv_tcp_non_b\n");
        if(tmp < 0) {
        	if(errno == EAGAIN) {
        		usleep(50);
        		continue;
        	}
        	else {
 				perror("receive failed in Recv_tcp_non_b!\n");
				exit(0);       		
        	}
        }
        else if(tmp == len_specified) {
/*
			if(len != len_specified) {
				printf("  ");
				for(int i = 0; i < len_specified; i++) {
					printf("%c", *(loc+i));
				}
				printf("\n");
			}
*/
        	break;
        }
        
        if(tmp >= 0) {
/*
            printf("  ");
            for(int i = 0; i < tmp; i++) {
                printf("%c", *(loc+i));
            }   
*/
   	    	len_specified -= tmp;
        	loc += tmp;
    	}
    }
//	printf("\nrecv %d bytes\n", len);

    return len;
}


int Transmitter::
Sendto_tcp(char *data, int len) {
	int len_sent = 0;
	len_sent = sendto(sock_id, data, len, 0, (SA *)&server_addr,
					  server_addr_len);
	if(-1 == len_sent) {
		perror("sendto failed");
		exit(0);
	}
	else if(len_sent != len) {
		printf("\nOnly send %dbytes data\n", len_sent);
	}
	return len_sent;
}

int Transmitter::
Recvfrom_tcp(char *data_dst, int len) {
	int len_recv;
	len_recv = recvfrom(sock_id, data_dst, len, 0, (SA *)&server_addr, 
						&server_addr_len);
	if(-1 == len_recv) {
		perror("\nrecv failed\n");
		exit(0);
	}
	else if(len_recv != len) {
//		printf("\nOnly recv %dbytes data\n", len_recv);
	}
	return len_recv;
}


void Transmitter::
recv_td_func(int id_core, int id_path, Data_Manager &data_manager) {

	affinity_set(id_core, "recv_thread");

	int cnt_pkt    = 0;
	int cnt_symbol = 0;
	
	int cnt_codeSym = 0;
	int cnt_dataSym = 0;

//	uchar prev_seg_id    = INIT_VAL;
	uchar prev_block_id  = INIT_VAL;
//	uchar symbol_id_prev = INIT_VAL;

//in case of the error of no decalration
	shared_ptr<struct Block_Data> block_data = nullptr;

	uchar id_seg = 0; uchar id_region = 0; uchar block_id = 0; 
	uchar symbol_id = 0; uchar s_level = 0; uchar k_fec= 0; uchar m_fec = 0;
	int originBlk_size = 0;;

	VData_Type packet[SYMBOL_LEN_FEC + LEN_CONTRL_MSG];

	
//	PRINT_PROCEDURE("waiting to enter the while recycle of recv_thread!");
	while(1) {
//		PRINT_PROCEDURE("already entering the while recycle!");
		memset(packet, 0, SYMBOL_LEN_FEC + LEN_CONTRL_MSG);
		
//		PRINT_PROCEDURE("Wait for receiving pkts!");
		Recv_tcp_non_b_fixed_len(packet, SYMBOL_LEN_FEC + LEN_CONTRL_MSG);
//		printf("*received the %d-th packet*\n", ++cnt_pkt);

		if(Terminal_AllThds || Terminal_RecvThds) {
			break;
		}
//notice the setTimer thread to start the timer
//		if(Flag_AlreadRecv != YES) {Flag_AlreadRecv = YES;}
		if(DATA_PKT == packet[0]) {
//			PRINT_PROCEDURE("DATA == packet[0]");
			decaps_pkt(packet, id_seg, id_region, block_id, symbol_id,
					   originBlk_size, s_level, k_fec, m_fec);
//			if(id_seg == prev_seg_id) {
				if(block_id != prev_block_id) {
//					PRINT_PROCEDURE("block_id != prev_block_id");
//psuh block_data into recvQ_data[id_path], if not the packet of first block 
					if(INIT_VAL != prev_block_id) {
//						PRINT_PROCEDURE("INIT_VAL != prev_block_id");
						block_data->cnt_s = cnt_symbol;
						data_manager.recvQ_data[id_path].push(block_data);
						printf("successfully pushed a block data into recvQ\n");
/*
						printf("pushed data is as folowing:\n");
						for(int i = 0; i < cnt_symbol; i++) {
							for(int k = 0; k < block_data->S_FEC; k++) {
								printf("%c", block_data->data[i][k]);
							}
						}
						printf("\n");
*/
						Print_BlockData(block_data);	
						cnt_symbol = 0;
					}
					prev_block_id = block_id;
// initialize the block data ,preparing for the new block
					block_data = (shared_ptr<struct Block_Data>) new(struct Block_Data);

					block_data->data = MALLOC(VData_Type *, 256);

					block_data->erasure = MALLOC(int, k_fec+m_fec);
					for(int i = 0; i < k_fec + m_fec; i++) {
						block_data->erasure[i] = LOST;
					}
					
//					if(s_level == 0) {block_data->S_FEC = SYMBOL_LEN_FEC;}
//					else {printf("s_level = %d,  fault happened!!!\n", s_level);}
					block_data->block_id = block_id;
					
					block_data->S_FEC = SYMBOL_LEN_FEC;

					block_data->K_FEC = k_fec;
					block_data->M_FEC = m_fec;
					block_data->originBlk_size = originBlk_size;
					block_data->id_seg = id_seg;
					block_data->id_region = id_region;

//					Print_BlockData(block_data);
					
					VData_Type *symbol = MALLOC(VData_Type, block_data->S_FEC);
					memcpy(symbol, &(packet[LEN_CONTRL_MSG]), block_data->S_FEC);
					block_data->data[cnt_symbol] = symbol;

					cnt_symbol++;
//					printf("the cnt symbols is %d\n", cnt_symbol);
					block_data->erasure[symbol_id] = GET;
//					printf("\nthe symbol_id is equal to %d\n", block_data->erasure[symbol_id]);
				}
				else {
//					PRINT_PROCEDURE("else if(block_id == prev_block_id)");
					VData_Type *symbol = MALLOC(VData_Type, block_data->S_FEC);
					memcpy(symbol, &(packet[LEN_CONTRL_MSG]), block_data->S_FEC);
					block_data->data[cnt_symbol] = symbol;
					
					cnt_symbol++;
//					printf("the cnt symbols is %d\n", cnt_symbol);
					block_data->erasure[symbol_id] = GET;
				}
//
//				Print_BlockData(block_data);
		}
		else if(STOP_PKT == packet[0]){
			printf("already received a VSegment terminal packet\n");
			break;
		}

		else if(CTRL_PKT == packet[0]) {
			
		}
	}
//push the last one into recv queue!!!	
	block_data->cnt_s = cnt_symbol;
	data_manager.recvQ_data[id_path].push(block_data);
	printf("successfully pushed a block data into recvQ\n");
	Print_BlockData(block_data);
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
	int type_pkt  = (uchar)packet[0];

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
/*
	printf("the packet info is:\n");
	printf("type_pkt = %d, id_seg = %d, id_region = %d, originBlk_size = %d, block_id = %d, symbol_id = %d, \
		s_level = %d, k_fec = %d, m_fec = %d\n", type_pkt, id_seg, id_region, originBlk_size, \
		block_id, symbol_id, s_level, k_fec, m_fec);
*/
}


void Transmitter::
Print_BlockData(shared_ptr<struct Block_Data> blk) {
	printf("The %d-th block is as following:\n", blk->block_id);
	printf("Block( id_seg = %d, id_region = %d, S_FEC = %d, K_FEC = %d, M_FEC = %d, \
			cnt_s = %d, originBlk_size = %d\n", blk->id_seg, blk->id_region, blk->S_FEC, blk->K_FEC, blk->M_FEC, blk->cnt_s, blk->originBlk_size);
	printf("\nthe erasure array is as following:\n");
	for(int i = 0; i < (blk->K_FEC+blk->M_FEC); i++) {
		printf("%d ", blk->erasure[i]);
	}
	printf("\n");
}

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
