#include "../include/common.h"
#include "../include/system_params.h"
#include "../include/bitrate_select.h"


//for debugging
#define DEBUG_BITRATE_SELECTOR


Bitrate_Selector::Bitrate_Selector(double _bitrate[BITRATE_TYPE_NUM]) {
	for(int i = 0; i < BITRATE_TYPE_NUM; i++) {
		bitrate[i] = _bitrate[i];
	}
}

//modified by shawnshanks_fei, in 20180316.am 
//select bitrate types for each region of a video segment
void Bitrate_Selector::setBitrate(int tile_num[REGION_NUM], 
								  Channel_Inf chan_inf[NUM_PATH], 
								  Video_Reader &video_reader) {
	double bw_residual;

	for(int k = 0; k < REGION_NUM; k++) {
		video_reader.bitrate_decs[k] = bitrate[LOWEST];	
	}
	printf("\nThe bitrate is as following:\n");
	for(int i = 0; i < REGION_NUM; i++) {
		printf("%lf ", video_reader.bitrate_decs[i]);
	}
	bw_residual = chan_inf[0].avail_bw*(1-chan_inf[0].plr) + 
				  chan_inf[1].avail_bw*(1-chan_inf[1].plr) - 
				  bitrate[LOWEST];
//	if(bw_residual < (bitrate[MEDIAN]/TILE_NUM_TOTAL*tile_num[HIGHEST])) {
//		return;
//	}
//start from the FOV region, which features highest priority to outmostregion
	for(int i = 0; i < REGION_NUM; i++){
//start selecting from the highest quality
		bw_residual += video_reader.bitrate_decs[i];
		for(int k = 0; k < BITRATE_TYPE_NUM-1; k++) {
			if(bw_residual > (bitrate[k] / TILE_NUM_TOTAL * tile_num[i])) {
				video_reader.bitrate_decs[i] = bitrate[k];
				break;
			}
		}
		bw_residual -= video_reader.bitrate_decs[i];
	}

#ifdef DEBUG_BITRATE_SELECTOR
	printf("\nThe bitrate is as following:\n");
	for(int i = 0; i < REGION_NUM; i++) {
		printf("%lf ", video_reader.bitrate_decs[i]);
	}
	printf("\n");
#endif
}