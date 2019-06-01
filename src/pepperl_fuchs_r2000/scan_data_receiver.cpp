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

#include "scan_data_receiver.h"

#include <ctime>
#include <unistd.h>

#if __cplusplus>=201103
	#include <thread>
#else
	#include "Poco/ScopedLock.h"
#endif


namespace pepperl_fuchs {

	void runner(ScanDataReceiver& recv) {
		recv.run();
	}
	
//-----------------------------------------------------------------------------
ScanDataReceiver::ScanDataReceiver():
    scan_data_()
{
	data_buffer_.resize(65536);
    last_data_time_ = std::time(0);
    is_connected_ = false;
}


//-----------------------------------------------------------------------------
bool ScanDataReceiver::handleNextPacket()
{
    // Search for a packet
    int packet_start = findPacketStart();
    if( packet_start<0 )
        return false;

    // Try to retrieve packet
    char buf[65536];
    PacketTypeC* p = (PacketTypeC*) buf;
    if( !retrievePacket(packet_start,p) )
        return false;

	
	// Lock internal outgoing data queue, automatically unlocks at end of function
#if __cplusplus>=201103
	std::unique_lock<std::mutex> lock(data_mutex_);
#else
    Poco::ScopedLock<Poco::Mutex> lock(data_mutex_);
#endif
	
    // Create new scan container if necessary
    if( p->header.packet_number == 1 || scan_data_.empty() )
    {
		
#if __cplusplus>=201103
        scan_data_.emplace_back();
#else
		scan_data_.push_back(ScanData());
#endif
        if( scan_data_.size() > 100 )
        {
            scan_data_.pop_front();
            std::cerr << "Too many scans in receiver queue: Dropping scans!" << std::endl;
        }
    }
    
    ScanData& scandata = scan_data_.back();

    // Parse payload of packet
    uint32_t* p_scan_data = (uint32_t*) &buf[p->header.header_size];
    int num_scan_points = p->header.num_points_packet;

    for( int i=0; i<num_scan_points; i++ )
    {
        unsigned int data = p_scan_data[i];
        unsigned int distance = (data & 0x000FFFFF);
        unsigned int amplitude = (data & 0xFFFFF000) >> 20;

        scandata.distance_data.push_back(distance);
        scandata.amplitude_data.push_back(amplitude);
    }

    // Save header
    scandata.headers.push_back(p->header);
	
    return true;
}

//-----------------------------------------------------------------------------
int ScanDataReceiver::findPacketStart()
{
    if( ring_buffer_.size()<60 )
        return -1;
    for( std::size_t i=0; i<ring_buffer_.size()-4; i++)
    {
		if(   ((unsigned char) ring_buffer_[i])   == 0x5c
           && ((unsigned char) ring_buffer_[i+1]) == 0xa2
           && ((unsigned char) ring_buffer_[i+2]) == 0x43
           && ((unsigned char) ring_buffer_[i+3]) == 0x00 )
        {
            return i;
        }
    }
    return -2;
}

//-----------------------------------------------------------------------------
bool ScanDataReceiver::retrievePacket(std::size_t start, PacketTypeC* p)
{
    if( ring_buffer_.size()<60 )
        return false;

    // Erase preceding bytes
	while (start > 0) {
		ring_buffer_.pop_front();
		start--;
	}
	
    char* pp = (char*) p;
    // Peek from header (leave header in the ringbuffer for now)
    // first 60 bytes
	for (int i=0; i<60; i++) {
		pp[i] = ring_buffer_[i];
	}

    if( ring_buffer_.size() < p->header.packet_size )
        return false;

    // Read header+payload data (removes data from ringbuffer)
    readBufferFront(pp,p->header.packet_size);

    return true;
}

//-----------------------------------------------------------------------------
bool ScanDataReceiver::checkConnection()
{
    if( !isConnected() )
        return false;
    if( (std::time(0)-last_data_time_) > 2 )
    {
        disconnect();
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
ScanData ScanDataReceiver::getScan()
{
#if __cplusplus>=201103
	std::unique_lock<std::mutex> lock(data_mutex_);
	ScanData data(std::move(scan_data_.front()));
	scan_data_.pop_front();
	return data;
#else
    Poco::ScopedLock<Poco::Mutex> lock(data_mutex_);
	
	// safety
	if (scan_data_.size() == 0) {
		return ScanData();
	}
	
    // get data
    ScanData data(scan_data_.front());
    // remove data from front of queue
    scan_data_.pop_front();
	
	return data;
#endif
}

//-----------------------------------------------------------------------------
std::size_t ScanDataReceiver::getScansAvailable()
{
	// lock to ensure we get correct size
#if __cplusplus>=201103
	std::unique_lock<std::mutex> lock(data_mutex_);
#else
	Poco::ScopedLock<Poco::Mutex> lock(data_mutex_);
#endif
	
	return scan_data_.size();
}
	
//-----------------------------------------------------------------------------
std::size_t ScanDataReceiver::getFullScansAvailable() const
{
    if( scan_data_.size() == 0 )
        return 0;
    else
        return scan_data_.size()-1;
}
	
//-----------------------------------------------------------------------------
void ScanDataReceiver::writeBufferBack(char *src, std::size_t numbytes)
{
    if( ring_buffer_.size()+numbytes > ring_buffer_.size() )
        throw std::exception();

    // lock mutex
	std::lock_guard<std::mutex> lock(ring_buffer_mutex_);
    // append data to rungbuffer
	
	for (int i=0; i < numbytes; i++) {
		ring_buffer_.push_back(src[i]);
	}
}

//-----------------------------------------------------------------------------
void ScanDataReceiver::readBufferFront(char *dst, std::size_t numbytes)
{
    if( ring_buffer_.size() < numbytes )
        throw std::exception();
    
    // lock mutex
    std::lock_guard<std::mutex> lock(ring_buffer_mutex_);
    
    // read buffer from ringbuffer
	for (int i=0; i < numbytes; i++) {
		dst[i] = ring_buffer_.front();
		ring_buffer_.pop_front();
	}	
}

//-----------------------------------------------------------------------------
}
