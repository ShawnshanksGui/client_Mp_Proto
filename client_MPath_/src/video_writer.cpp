#include <string>

#include "../include/video_writer.h"
#include "../include/system_params.h"
#include "../include/data_manager.h"
#include "../include/myUtility.h"
#include "../include/common.h"

//idendifies whether the first time or not
#define INIT_VAL_W 10000

using namespace std;

//notice whether ought to close the current thread or not
extern int Terminal_AllThds;
extern int Terminal_WriterThds;

void Video_Writer::
video_writer_td_func(int id_path, Data_Manager &data_manager) {

	int len_seg = 1;

//	int cnt_file = 0;
	int cur_id_seg     = INIT_VAL_W;
	int cur_id_region  = INIT_VAL_W;

//	int cur_id_region_arr[REGION_NUM]   = INIT_VAL_W;
	int prev_id_seg_arr[REGION_NUM][2] = {{INIT_VAL_W, INIT_VAL_W},
										  {INIT_VAL_W, INIT_VAL_W},
										  {INIT_VAL_W, INIT_VAL_W}};

	FILE *fp[REGION_NUM][2] = {{nullptr, nullptr}, {nullptr, nullptr}, \
							   {nullptr, nullptr}};

	while(!Terminal_AllThds && !Terminal_WriterThds) {

		if(data_manager.decdQ_data[id_path].size() > 0) {

			shared_ptr<struct Block_Decd> block_decd = \
			data_manager.decdQ_data[id_path].front();
			data_manager.decdQ_data[id_path].pop();

			cur_id_seg    = block_decd->id_seg;
			cur_id_region = block_decd->id_region;

			if(cur_id_seg != prev_id_seg_arr[cur_id_region][cur_id_seg%2]) {
				prev_id_seg_arr[cur_id_region][cur_id_seg%2] = cur_id_seg;
//create and open a new file.
//================================================================================
				string outputVideo_path;
				outputVideo_path = "~/360video_received/len_seg_"+to_string(len_seg) \
								    + "s/seg_" + to_string(cur_id_seg) + "_region_"\
								    + to_string(cur_id_region) + ".265";

		   		char *out_path_cStr=(char *)new char[outputVideo_path.length()+1];
   				strcpy(out_path_cStr, outputVideo_path.c_str());

				Fopen_for_write(&(fp[cur_id_region][cur_id_seg%2]), out_path_cStr);
				delete [] out_path_cStr;
//================================================================================
			}
//write the data of the currentn block into file, which file descripter 
//is fp[cur_id_region][cur_id_seg%2]);)
			Fwrite(block_decd->data_decd, block_decd->len_remain_data, \
					   fp[cur_id_region][cur_id_seg%2]);

			SAFE_FREE(block_decd->data_decd);	
		}
	}
}