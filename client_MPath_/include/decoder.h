#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <memory>

#include "data_manager.h"

class Decoder {
public:
	Decoder() {}
	~Decoder() {}

	void decoder_init();
	void *decode(char *data_src, int S, int K, int M);
	void decoder_td_func(int id_core, int id_path, Data_Manager &data_manager);
	VData_Type *extract_origin_data(shared_ptr<struct Block_Data> \
									block_data);	
	void BLOCK_FREE(shared_ptr<struct Block_Data> block_data);

	void encaps_decdBlk(shared_ptr<struct Block_Decd> block_decd,
						shared_ptr<struct Block_Data> block_data, 
						VData_Type *decd_data, int _size);
private:

};
#endif