#include <string>

#include "../include/system_params.h"
#include "../include/video_writer.h"
#include "../include/data_manager.h"

#include "../include/myUtility.h"

using namespace std;

void Video_Writer::
video_writer_td_func(int id_path, Data_Manager &data_manager) {
	int cnt_file = 0;

	int cur_region      = INIT_VAL;

	int cur_id_seg      = INIT_VAL;
	int prev_id_seg     = INIT_VAL;

//	int cur_id_region_arr[REGION_NUM]   = INIT_VAL;
	int prev_id_region_arr[REGION_NUM]  = INIT_VAL;


	FILE *fp[REGION_NUM] = {nullptr, nullptr, nullptr};


	char *outputVideo_path = nullptr;

	while(1) {
		shared_ptr<struct Block_Decd> block_decd = \
		data_manager.decdQ_data[id_path].front();
		data_manager.decdQ_data[id_path].pop();

		cur_id_seg    = block_decd.id_seg;
		cur_id_region = block_decd.id_region;

		if(cur_id_seg != prev_id_seg) { //|| cur_id_region != prev_id_region) {
//			if(cur_id_region[cur_id_region] != prev_id_seg) {
			for(int i = 0; i < REGION_NUM; i++) {
				string outputVideo_path;
				outputVideo_path = "seg_" + to_string(id_seg) + "_region_" \
									to_string(i) + ".265";
				out_path_c_str = outputVideo_path.c_str();
				Fopen_for_write(&fp[cur_id_region], out_path_c_str);
			}
		}
	}
}