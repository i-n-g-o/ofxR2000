//
//  scan_data_receiver_udp.cpp
//	Ingo Randolf, 2015
//
//	https://github.com/i-n-g-o/ofxR2000
//
//
//	udp data receiver using poco socket
//	based on the modified class ScanDataReceiver by pepperl+fuchs
//

#include "scan_data_receiver_udp.h"

namespace pepperl_fuchs {

    ScanDataReceiverUDP::ScanDataReceiverUDP() :
        ScanDataReceiver()
	    ,udp_port_(-1)
		,udp_socket(Poco::Net::SocketAddress(Poco::Net::IPAddress(Poco::Net::IPAddress::IPv4), 0))
    {
		udp_port_ = udp_socket.address().port();

		is_connected_ = true;
		
#if __cplusplus>=201103
		io_service_thread_ = std::thread(runner, std::ref(*this));
#else
        io_service_thread_.start(*this);
#endif
		
//        std::cout << "Receiving scanner data at local UDP port " << udp_port_ << " ... ";
    }
    
    
    ScanDataReceiverUDP::~ScanDataReceiverUDP()
    {
        disconnect();
    }
    
    
    void ScanDataReceiverUDP::run()
    {
		Poco::Net::SocketAddress sender;
		char* buffer = data_buffer_.data();
		
#if __cplusplus>=201103
		isRunning = true;
		
		while(isRunning)
#else
		bool doIt = true;
		
		run_mutex.lock();
		isRunning = true;
		run_mutex.unlock();
		
		
		// thread worker
		while (doIt)
#endif
		{
            // do
			std::size_t numBytes = udp_socket.receiveFrom(buffer, data_buffer_.size(), sender);

            writeBufferBack(buffer, numBytes);
            
            while( handleNextPacket() ) {}
			
#if __cplusplus<201103
            run_mutex.lock();
            doIt = isRunning;
            run_mutex.unlock();
#endif
        }

        // thread done
    }

    
    void ScanDataReceiverUDP::disconnect()
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
		
		udp_socket.close();        
    }
}