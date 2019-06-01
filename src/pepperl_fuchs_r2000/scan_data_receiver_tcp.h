//
//  scan_data_receiver_tcp.h
//	Ingo Randolf, 2015
//
//	https://github.com/i-n-g-o/ofxR2000
//
//
//	tcp data receiver using poco socket
//	based on the modified class ScanDataReceiver by pepperl+fuchs
//

#ifndef SCAN_DATA_RECEIVER_TCP_H
#define SCAN_DATA_RECEIVER_TCP_H

#include <stdio.h>

#include "ofxTCPManager.h"

#include "scan_data_receiver.h"

namespace pepperl_fuchs
{
	class ScanDataReceiverTCP: public ScanDataReceiver
	{
	public:
		//! Connect synchronously to the given IP and TCP port and start reading asynchronously
		ScanDataReceiverTCP(const std::string hostname, const int tcp_port);
		~ScanDataReceiverTCP();
		
		void disconnect();
		
		//! do threaded work here
		void run();
		
		
	private:
		ofxTCPManager tcp_socket;
	};
}
#endif /* defined(SCAN_DATA_RECEIVER_TCP_H) */
