#ifndef _VIDEO_WRITER_H_
#define _VIDEO_WRITER_H_

#include "data_manager.h"

class Video_Writer{
public:
	Video_Writer() {}
	~Video_Writer() {}
	
	void video_writer_td_func(int id_path, Data_Manager &data_manager);

private:

};


#endif