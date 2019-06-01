// Copyright (c) 2014, Pepperl+Fuchs GmbH, Mannheim
// Copyright (c) 2014, Denis Dillenberger
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or
//   other materials provided with the distribution.
//
// * Neither the name of Pepperl+Fuchs nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//
// openFrameworks addon by Ingo Randolf, 2015
// https://github.com/i-n-g-o/ofxR2000
//


#include <ctime>
#include <algorithm>
#include "r2000_driver.h"
#include "http_command_interface.h"

#include "scan_data_receiver.h"
#include "scan_data_receiver_udp.h"
#include "scan_data_receiver_tcp.h"


namespace pepperl_fuchs
{

	R2000Driver::R2000Driver()
	{
		command_interface_ = 0;
		data_receiver_ = 0;
		is_connected_ = false;
		is_capturing_ = false;
		watchdog_feed_time_ = 0;
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::connect(const std::string hostname, int port)
	{
//		std::cout << "connect to: " << hostname << std::endl;
		
		command_interface_ = new HttpCommandInterface(hostname, port);
		
		optional<ProtocolInfo> opi = command_interface_->getProtocolInfo();
		
		if (!opi.isSpecified() ||
			(opi.value()).version_major != 1)
		{
			std::cerr << "ERROR: Could not connect to laser range finder!" << std::endl;
			return false;
		}

		if (opi.value().version_major != 1 )
		{
			std::cerr << "ERROR: Wrong protocol version (version_major=" << (opi.value()).version_major << ", version_minor=" << (opi.value()).version_minor << ")" << std::endl;
			return false;
		}
		
		protocol_info_ = opi.value();    
		parameters_ = command_interface_->getParameters(command_interface_->getParameterList());
		is_connected_ = true;
		return true;
	}

	//-----------------------------------------------------------------------------
	R2000Driver::~R2000Driver()
	{
		disconnect();
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::startCapturingTCP()
	{
		if(!checkConnection()) {
			return false;
		}

		if (data_receiver_) {
			delete data_receiver_;
			data_receiver_ = 0;
		}
		
		handle_info_ = command_interface_->requestHandleTCP();
		if( !handle_info_.isSpecified() )
			return false;

		data_receiver_ = (ScanDataReceiver*)new ScanDataReceiverTCP(handle_info_.value().hostname, handle_info_.value().port);
		
		if(!data_receiver_->isConnected() ||
		   !command_interface_->startScanOutput(handle_info_.value().handle))
		{
			return false;
		}

		
		food_timeout_ = floor(std::max((handle_info_.value().watchdog_timeout/1000.0/3.0),1.0));
		is_capturing_ = true;
		return true;
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::startCapturingUDP()
	{
		if (!checkConnection()) {
			return false;
		}

		if (data_receiver_) {
			delete data_receiver_;
			data_receiver_ = 0;
		}
		
		data_receiver_ = (ScanDataReceiver*)new ScanDataReceiverUDP();
		
		if (!data_receiver_->isConnected()) {
			return false;
		}
		
		// get udp port
		int udp_port = ((ScanDataReceiverUDP*)data_receiver_)->getUDPPort();
		
		handle_info_ = command_interface_->requestHandleUDP(udp_port);
		
		if (!handle_info_.isSpecified() ||
			!command_interface_->startScanOutput(handle_info_.value().handle))
		{
			return false;
		}

		food_timeout_ = std::floor(std::max((handle_info_.value().watchdog_timeout/1000.0/3.0),1.0));
		is_capturing_ = true;
		return true;
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::stopCapturing()
	{
		if( !is_capturing_ || !command_interface_ )
			return false;

		bool return_val = checkConnection();

		// safety
		if (data_receiver_) {
			delete data_receiver_;
			data_receiver_ = 0;
		}
		
		return_val = return_val && command_interface_->stopScanOutput(handle_info_.value().handle);

		is_capturing_ = false;
		return_val = return_val && command_interface_->releaseHandle(handle_info_.value().handle);
		handle_info_ = Poco::Optional<HandleInfo>();
		return return_val;
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::checkConnection()
	{
		if( !command_interface_ || !isConnected() || !command_interface_->getProtocolInfo().isSpecified() )
		{
			std::cerr << "ERROR: No connection to laser range finder or connection lost!" << std::endl;
			return false;
		}
		return true;
	}

	//-----------------------------------------------------------------------------
	ScanData R2000Driver::getScan()
	{
		feedWatchdog();
		
		if( data_receiver_ )
			return data_receiver_->getScan();
		else
		{
			std::cerr << "ERROR: No scan capturing started!" << std::endl;
			return ScanData();
		}
		return ScanData();
	}


	//-----------------------------------------------------------------------------
	std::size_t R2000Driver::getScansAvailable() const
	{
		if( data_receiver_ )
			return data_receiver_->getScansAvailable();
		else
		{
			std::cerr << "ERROR: No scan capturing started!" << std::endl;
			return 0;
		}
	}

	//-----------------------------------------------------------------------------
	std::size_t R2000Driver::getFullScansAvailable() const
	{
		if( data_receiver_ )
			return data_receiver_->getFullScansAvailable();
		else
		{
			std::cerr << "ERROR: No scan capturing started!" << std::endl;
			return 0;
		}
	}

	//-----------------------------------------------------------------------------
	void R2000Driver::disconnect()
	{
		if( isCapturing() )
			stopCapturing();

		if (data_receiver_)
			delete data_receiver_;
		
		if (command_interface_)
			delete command_interface_;

		data_receiver_ = 0;
		command_interface_ = 0;

		is_capturing_ = false;
		is_connected_ = false;

		handle_info_ = Poco::Optional<HandleInfo>();
		protocol_info_ = ProtocolInfo();
		parameters_ = std::map< std::string, std::string >();
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::isCapturing()
	{
		return is_capturing_ && data_receiver_->isConnected();
		return false;
	}

	//-----------------------------------------------------------------------------
	const std::map< std::string, std::string >& R2000Driver::getParameters()
	{
		if( command_interface_ )
			parameters_ = command_interface_->getParameters(command_interface_->getParameterList());
		return parameters_;
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::setScanFrequency(unsigned int frequency)
	{
		if( !command_interface_ )
			return false;
		return command_interface_->setParameter("scan_frequency",Poco::NumberFormatter::format(frequency));
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::setSamplesPerScan(unsigned int samples)
	{
		if( !command_interface_ )
			return false;
		
		return command_interface_->setParameter("samples_per_scan",Poco::NumberFormatter::format(samples));
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::rebootDevice()
	{
		if( !command_interface_ )
			return false;
		return command_interface_->rebootDevice();
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::resetParameters(const std::vector<std::string> &names)
	{
		if( !command_interface_ )
			return false;
		return command_interface_->resetParameters(names);
	}

	//-----------------------------------------------------------------------------
	bool R2000Driver::setParameter(const std::string &name, const std::string &value)
	{
		if( !command_interface_ )
			return false;
		return command_interface_->setParameter(name,value);
	}

	//-----------------------------------------------------------------------------
	void R2000Driver::feedWatchdog(bool feed_always)
	{
		const double current_time = std::time(0);

		if( (feed_always || watchdog_feed_time_<(current_time-food_timeout_)) && handle_info_.isSpecified() && command_interface_  )
		{
			if( !command_interface_->feedWatchdog(handle_info_.value().handle) )
				std::cerr << "ERROR: Feeding watchdog failed!" << std::endl;
			watchdog_feed_time_ = current_time;
		}
	}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
} // namespace
