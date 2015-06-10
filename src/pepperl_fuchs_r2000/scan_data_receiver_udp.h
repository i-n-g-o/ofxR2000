//
//  scan_data_receiver_udp.h
//	Ingo Randolf, 2015
//
//	https://github.com/i-n-g-o/ofxR2000
//
//
//	udp data receiver using poco socket
//	based on the modified class ScanDataReceiver by pepperl+fuchs
//

#ifndef SCAN_DATA_RECEIVER_UDP_H
#define SCAN_DATA_RECEIVER_UDP_H

#include <stdio.h>

#include "Poco/Thread.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/DatagramSocket.h"

#include "scan_data_receiver.h"

namespace pepperl_fuchs {
	
	class ScanDataReceiverUDP: public ScanDataReceiver
	{
	public:
		//! Open an UDP port and listen on it
		ScanDataReceiverUDP();
		~ScanDataReceiverUDP();
		
		void disconnect();
		
		//! do threaded work here
		void run();
		
		//! Get open and receiving UDP port
		int getUDPPort() const { return udp_port_; }
		
		
	private:
		//! Data (UDP) port at local side
		int udp_port_;
		
		//! Data (UDP) socket
		Poco::Net::DatagramSocket udp_socket;
	};
	
}
#endif /* defined(SCAN_DATA_RECEIVER_UDP_H) */
