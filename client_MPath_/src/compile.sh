
#gcc -std=gnu99  -g -o test sender.c utility.c mySocket.c thread_core_affinity_set.c -lpthread
#g++ -std=c++11 -g -o test data_manager.cpp server.cpp mySocket.cpp utility.cpp thread_core_affinity_set.cpp -lpthread

g++ -std=c++11 -g -o test_writer main.cpp video_writer.cpp  data_manager.cpp myUtility.cpp decoder.cpp mySocket.cpp timer.cpp libRS_FFT.a thread_core_affinity_set.cpp -lpthread