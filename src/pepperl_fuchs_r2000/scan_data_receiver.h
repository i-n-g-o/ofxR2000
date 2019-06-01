// Copyright (c) 2014, Pepperl+Fuchs GmbH, Mannheim
// Copyright (c) 2014, Denis Dillenberger
// All rights reserved.
//
// Use, modification, and distribution is subject to the
// 3-clause BSD license ("Revised BSD License",
// "New BSD License", or "Modified BSD License")
// You should have received a copy of this license
// in a file named COPYING or LICENSE.

//
// openFrameworks addon by Ingo Randolf, 2015
// https://github.com/i-n-g-o/ofxR2000
//


#ifndef SCAN_DATA_RECEIVER_H
#define SCAN_DATA_RECEIVER_H

#include <string>
#include <iostream>
#include <deque>
#include <queue>

#include "ofFileUtils.h"

#if __cplusplus>=201103
	#include <mutex>
	#include <thread>
	#include <atomic>
	#include "packet_structure_cpp11.h"
#else
	#include "Poco/Mutex.h"
	#include "Poco/Thread.h"
	#include "packet_structure.h"
#endif

namespace pepperl_fuchs {

//! \class ScanDataReceiver
//! \brief Receives data of the laser range finder via IP socket
//! Receives the scanner data with asynchronous functions of the Boost::Asio library
class ScanDataReceiver
{
public:
    //! Connect synchronously to the given IP and TCP port and start reading asynchronously
    ScanDataReceiver(const std::string hostname, const int tcp_port);

    //! Open an UDP port and listen on it
    ScanDataReceiver();
    
    //! Return connection status
    bool isConnected() const { return is_connected_; }

    //! Disconnect and cleanup
    virtual void disconnect() = 0;

    //! Pop a single scan out of the internal FIFO queue
    //! CAUTION: Returns also unfinished scans for which a full rotation is not received yet
    //! Call getFullScansAvailable() first to see how many full scans are available
    //! @returns A ScanData struct with distance and amplitude data as well as the packet headers belonging to the data
    ScanData getScan();

    //! Get the total number of laserscans available (even scans which are not fully reveived yet)
	std::size_t getScansAvailable();

    //! Get the total number of fully received laserscans available
    std::size_t getFullScansAvailable() const;

    virtual void run() = 0;
    
protected:
#if __cplusplus>=201103
	std::thread io_service_thread_;
#else
    Poco::Thread io_service_thread_;
#endif
	
    //! Internal connection state
    bool is_connected_;
    
    //! Try to read and parse next packet from the internal ring buffer
    //! @returns True if a packet has been parsed, false otherwise
    bool handleNextPacket();
    
    //! Search for magic header bytes in the internal ring buffer
    //! @returns Position of possible packet start, which normally should be zero
    int findPacketStart();
    
    //! Try to read a packet from the internal ring buffer
    //! @returns True on success, False otherwise
    bool retrievePacket( std::size_t start, PacketTypeC* p );
    
    //! Checks if the connection is alive
    //! @returns True if connection is alive, false otherwise
    bool checkConnection();
    
    //! Read fast from the front of the internal ring buffer
    //! @param dst Destination buffer
    //! @numbytes Number of bytes to read
    void readBufferFront(char* dst, std::size_t numbytes );
    
    //! Write fast at the back of the internal ring buffer
    //! @param src Source buffer
    //! @numbytes Number of bytes to write
    void writeBufferBack(char* src, std::size_t numbytes );
    
    
    //! data Buffer
    ofBuffer data_buffer_;
	
#if __cplusplus>=201103
	std::atomic<bool> isRunning;
#else
	bool isRunning;
	Poco::FastMutex run_mutex;
#endif
	
private:
    //! Internal ringbuffer for temporarily storing reveived data
	std::deque<char> ring_buffer_;

    //! Protection against data races between ROS and IO threads
#if __cplusplus>=201103
	std::mutex data_mutex_;
	std::mutex ring_buffer_mutex_;
#else
    Poco::Mutex data_mutex_;
#endif

    //! Double ended queue with sucessfully received and parsed data, organized as single complete scans
    std::deque<ScanData> scan_data_;

    //! time in seconds since epoch, when last data was received
    double last_data_time_;
};

	void runner(ScanDataReceiver& recv);
}
#endif // SCAN_DATA_RECEIVER_H
