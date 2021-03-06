#include "../include/thread_core_affinity_set.h"

#include <string>
#include <memory>
#include <vector>

#include "../include/common.h"
#include "../include/data_manager.h"
#include "../include/decoder.h"

//notice whether ought to close the current thread or not
extern int Terminal_AllThds;
extern int Terminal_DecdThds;

//specidies whether FFT_RS or RS
#define ENABLE_FFT_RS

#ifdef ENABLE_FFT_RS 
extern "C"
{
	#include "../include/rs_fft.h"
}

using namespace std;

void Decoder::
decoder_init()  {
	init();
	init_dec();	
}
//RS-FFT
void *Decoder::
decode(VData_Type **data_recv, int *erasure, int S, int K, int M) {
	return fft_decode(data_recv, erasure, S, K);
}	

#elif
extern "C"
{
	#include "../include/rs.h"
}

void Decoder::
decoder_init()  {
	
}
//RS codes
void *Decoder::
decode(VData_Type **data_recv, int *erasure, int S, int K, int M) {
	return rs_decode(data_recv, erasure, S, K, M);
}	
#endif



void Decoder::
decoder_td_func(int id_core, int id_path, Data_Manager &data_manager) {

	affinity_set(id_core, "decoder_thread");

	int _size = 0;
	VData_Type *decd_block = nullptr;

	decoder_init();
	
	while(!Terminal_AllThds && !Terminal_DecdThds) {
//fetch one block from queue_recv. 
		if(data_manager.recvQ_data[id_path].size() > 0) {
			shared_ptr<struct Block_Data> block_data = \
			data_manager.recvQ_data[id_path].front();
			data_manager.recvQ_data[id_path].pop();
			if(block_data->cnt_s < block_data->K_FEC) {
				printf("the cnt_s is %d\n", block_data->cnt_s);
//				void ** result_arr;			
				struct Ret_Val result;
				result = extract_origin_data(block_data);
				printf("succesfully extract_origin_data once\n");
				decd_block = result.data;
				_size      = result.data_size;
			}
			else {
				decd_block=(VData_Type *)decode(block_data->data, block_data->erasure,
												block_data->S_FEC, block_data->K_FEC,
												block_data->M_FEC);
				printf("succesfully decode once\n");
//				assert(block_data->originBlk_size == block_data->S_FEC * block_data->K_FEC);
				_size = block_data->originBlk_size;
			}

			shared_ptr<Block_Decd> block_decd = (shared_ptr<struct Block_Decd>) \
												new(struct Block_Decd);
			encaps_decdBlk(block_decd, block_data, decd_block, _size);
			data_manager.decdQ_data[id_path].push(block_decd);
			printf("succesfully push a decd block into decQ_data once\n");
			BLOCK_FREE(block_data);
		}
	}
}


//==========================================================================
//==========================================================================
//Author:      shawnshanks_fei          Date:     
//Description: implement extract_origin_data, if having no enough data to decode
//Parameter:   code_pkt: indicates the number of 
//             code symbol(except for origin data)	       
//==========================================================================
Res_Val Decoder::
extract_origin_data(shared_ptr<struct Block_Data> block_data) {
#ifdef ENABLE_FFT_RS 
	int loc = 0;
//	vector<shared_ptr<void *>> result = new shared_ptr<void *>[2];
	Res_Val result;

//	VData_Type *data = MALLOC(VData_Type, block_data->S_FEC*block_data->K_FEC);
	VData_Type *data = MALLOC(VData_Type, block_data->originBlk_size);
	memset(data, 0, block_data->originBlk_size);

	int code_pkt = 0;
	for(int i = 0, k = 0; i < block_data->K_FEC+block_data->M_FEC; i++) {
		if(i < block_data->M_FEC) {
			if(GET == block_data->erasure[i]) {
				k++;
				code_pkt++;
			}
		}
		else{
			if((loc+block_data->S_FEC) < block_data->originBlk_size) {
//			if((loc+block_data->S_FEC) <= block_data->cnt_s*block_data->S_FEC){
				if(GET == block_data->erasure[i]) {
//					printf("the k = %d\n", k);
//					printf("");
					memcpy(&(data[(k-code_pkt)*(block_data->S_FEC)]), \
						   block_data->data[k], block_data->S_FEC);
					k++;
				}
			}
			else {
				int _len = block_data->originBlk_size - loc;
//				int _len = block_data->cnt_s*block_data->S_FEC - loc;
				if(GET == block_data->erasure[i]) {
					memcpy(&(data[(k-code_pkt)*(block_data->S_FEC)]), \
						   block_data->data[k], _len);
					loc += _len;
					break;					
				}
			}
			loc += block_data->S_FEC;
		}
	}
//get the addr of remaining origin data.
	result.data = data;
//get the number of remaining origin symbol(recovered), equal to loc.
	result.data_size = loc;
#elif
/*
................................................
................................................
*/	
#endif

	return result;
}

void Decoder::
BLOCK_FREE(shared_ptr<struct Block_Data> block_data) {
	for(int i = 0, k = 0; i < block_data->K_FEC+block_data->M_FEC; i++) {
		if(GET == block_data->erasure[i]) { 
			SAFE_FREE(block_data->data[k]);
			k++;
		}
	}
	SAFE_FREE(block_data->data);
	SAFE_FREE(block_data->erasure);
}


void Decoder::
encaps_decdBlk(shared_ptr<struct Block_Decd> block_decd,
			   shared_ptr<struct Block_Data> block_data, 
			   VData_Type *decd_data, int _size) {
	block_decd->data_decd        = decd_data;
	block_decd->len_remain_data  = _size;
	block_decd->id_seg           = block_data->id_seg;
	block_decd->id_region        = block_data->id_region;

}
