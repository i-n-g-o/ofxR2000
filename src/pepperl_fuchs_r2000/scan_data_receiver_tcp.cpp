//
//  scan_data_receiver_tcp.cpp
//	Ingo Randolf, 2015
//
//	https://github.com/i-n-g-o/ofxR2000
//
//
//	tcp data receiver using poco socket
//	based on the modified class ScanDataReceiver by pepperl+fuchs
//


#include "scan_data_receiver_tcp.h"


namespace pepperl_fuchs
{
	ScanDataReceiverTCP::ScanDataReceiverTCP(const std::string hostname, const int tcp_port) :
		ScanDataReceiver(),
		tcp_socket(Poco::Net::SocketAddress(hostname, tcp_port))
    {	
		is_connected_ = true;
		
		// start thread
#if __cplusplus>=201103
		io_service_thread_ = std::thread(runner, std::ref(*this));
#else
        io_service_thread_.start(*this);
#endif
    }
    
    
    ScanDataReceiverTCP::~ScanDataReceiverTCP()
    {
        disconnect();
    }
    
    
	void ScanDataReceiverTCP::run()
	{
		char* buffer = data_buffer_.data();
		
		// thread worker
#if __cplusplus>=201103
		isRunning = true;
		
		while(isRunning)
#else
		bool doIt = true;
			
		run_mutex.lock();
		isRunning = true;
		run_mutex.unlock();
			
		while(doIt)
#endif
		{
			// do
			std::size_t numBytes = tcp_socket.receiveBytes(buffer, data_buffer_.size());
			
			if (numBytes == 0) break; // gracefull shutdown...
			
			// write data to ringbuffer
			writeBufferBack(buffer, numBytes);
			
			// handle packets
			while( handleNextPacket() ) {}
			
#if __cplusplus<201103
            run_mutex.lock();
			doIt = isRunning;
            run_mutex.unlock();
#endif
		}
		
		// thread done
	}

    
    void ScanDataReceiverTCP::disconnect()
    {
        is_connected_ = false;
		
#if __cplusplus>=201103
        isRunning = false;
#else
		run_mutex.lock();
		isRunning = false;
		run_mutex.unlock();
#endif
		
        // wait until thread is done
        io_service_thread_.join();
		
        tcp_socket.close();
    }
}