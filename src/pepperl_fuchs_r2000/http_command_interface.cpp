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


#include "http_command_interface.h"

#include <iostream>

#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/RawSocket.h"
#include "Poco/Net/IPAddress.h"
#include "Poco/UnbufferedStreamBuf.h"

#include "Poco/Net/HTTPSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/DatagramSocket.h"
#include "Poco/URI.h"

#include "Poco/NumberFormatter.h"


namespace pepperl_fuchs {
	
	using namespace std;
	using namespace Poco;
	using namespace Poco::Net;
	
    //-----------------------------------------------------------------------------
	HttpCommandInterface::HttpCommandInterface(const std::string &http_host, int http_port)
    {
        http_host_ = http_host;
        http_port_ = http_port;
		http_status_code_ = Poco::Net::HTTPResponse::HTTP_NOT_FOUND;

    }
    
    
    //-----------------------------------------------------------------------------
	int HttpCommandInterface::httpGet(const std::string request_path, std::string &header, std::string &content)
    {
        header = "";
        content = "";
        
        try
		{
			HTTPClientSession httpSession(http_host_, http_port_);
			httpSession.setTimeout(Poco::Timespan(20,0));
			
			HTTPResponse res;
			
			// send request
			HTTPRequest req(HTTPRequest::HTTP_GET, request_path, HTTPMessage::HTTP_1_0);
			httpSession.sendRequest(req);
			
			// receive response header
			istream& response_stream = httpSession.receiveResponse(res);
			
			// get status code
			HTTPResponse::HTTPStatus status_code = res.getStatus();

			
			string http_version = res.getVersion();
			if (!response_stream || http_version.substr(0, 5) != "HTTP/")
			{
				std::cout << "Invalid response\n";
				return 0;
			}
			
			// get headers
			NameValueCollection::ConstIterator i = res.begin();
			while(i != res.end()) {
				header += i->first + "=" + i->second + "\n";
				i++;
			}
			
			
			if (status_code != HTTPResponse::HTTP_OK) {
				// dump headers
				std::cout << "headers: " << header;
				std::flush(std::cout);
			}
			
			
			std::string tmp;
			
			// read until eof
			while(!response_stream.eof()) {
				std::getline(response_stream, tmp);
				content += tmp;
			}
			
			
            
			// Substitute CRs by a space
			for( std::size_t i=0; i<header.size(); i++ ) {
                if( header[i] == '\r' )
                    header[i] = ' ';
			}
            
			for( std::size_t i=0; i<content.size(); i++ ) {
				if( content[i] == '\r' )
					content[i] = ' ';
			}

			return status_code;
			
		}
		catch (Poco::Exception & exc){
			std::cerr << "Exception: " <<  exc.displayText() << std::endl;			
			return 0;
		}
    }
    
    
    
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::sendHttpCommand(const std::string cmd, const std::map<std::string, std::string> param_values)
    {
        // Build request string
        std::string request_str = "/cmd/" + cmd + "?";
        
        
        
        for( std::map<std::string, std::string>::const_iterator kv = param_values.begin(); kv != param_values.end(); kv++ ) {
            request_str += kv->first + "=" + kv->second + "&";
        }
        
        if(request_str[request_str.size()-1] == '&' )
            request_str = request_str.substr(0,request_str.size()-1);
        
        // Do HTTP request
        std::string header, content;
        http_status_code_ = httpGet(request_str, header, content);
		
        // Try to parse JSON response
		// create parser
		
		// parse json
		std::stringstream ss(content);
		
        if (!jsonParser.parse(ss, root))
        {
			std::cerr << "Json::parse" << "Unable to parse string: " << jsonParser.getFormattedErrorMessages() << std::endl;
            return false;
        }
        
        // Check HTTP-status code
        if( http_status_code_ != 200 )
            return false;
        else
            return true;
    }
    
    
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::sendHttpCommand(const std::string cmd, const std::string param, const std::string value)
    {
        std::map<std::string, std::string> param_values;
        if( param != "" )
            param_values[param] = value;
        return sendHttpCommand(cmd,param_values);
    }
    
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::setParameter(const std::string name, const std::string value)
    {
        return sendHttpCommand("set_parameter",name,value) && checkErrorCode();
    }
    
    
    //-----------------------------------------------------------------------------
    Poco::Optional< std::string > HttpCommandInterface::getParameter(const std::string name)
    {
        if( !sendHttpCommand("get_parameter","list",name) || ! checkErrorCode()  )
            return Poco::Optional<std::string>();
        
        // query json for value
		
		if (!root.isMember(name)) {
			return Poco::Optional<std::string>();
		}
		
		Json::Value value = root.get(name, "");
		if (!value.isString())
			return Poco::Optional<std::string>();
		
        return Poco::Optional<std::string>(value.asString());
    }
    
    
    //-----------------------------------------------------------------------------
    std::map< std::string, std::string > HttpCommandInterface::getParameters(const std::vector<std::string> &names)
    {
        // Build request string
        std::map< std::string, std::string > key_values;
        std::string namelist;
        
        
        
        for( std::vector<std::string>::const_iterator s = names.begin(); s != names.end(); s++ ) {
            namelist += (*s + ";");
        }
        
        namelist.substr(0,namelist.size()-1);
        
        // Read parameter values via HTTP/JSON request/response
        if( !sendHttpCommand("get_parameter","list",namelist) || ! checkErrorCode()  )
            return key_values;
        
		
        
       
        // Extract values from JSON property_tree
        for( std::vector<std::string>::const_iterator s = names.begin(); s != names.end(); s++ )
        {
			if( root.isMember(*s) ) {
				
				Json::Value v = root[*s];
				try {
					
					if (v.isArray()) {
						
						std::string arrayString;
						for (int i=0; i<v.size()-1; i++) {
							arrayString += v[i].asString() + ", ";
						}
						arrayString += v[v.size()-1].asString();
						
						key_values[*s] = arrayString;
						
					} else if (v.isObject()) {
						key_values[*s] = "OBJECT";
					} else {
						key_values[*s] = v.asString();
					}
					
					
				} catch (Exception e) {
					std::cerr << "error: " << e.displayText() << std::endl;
				}
			} else {
                key_values[*s] = "--COULD NOT RETRIEVE VALUE--";
			}
        }
        
        return key_values;
    }
    
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::checkErrorCode()
    {
		Json::Value error_value = root["error_code"];
		Json::Value error_string = root["error_text"];
		
		if (!root.isMember("error_code") || !root["error_code"].isNumeric() || root["error_code"].asInt() != 0 || !root["error_text"].isString() || root["error_text"].asString() != "success") {
			
			if (root["error_text"].isString()) {
				std::cerr << "ERROR: scanner replied: " << root["error_text"].asString() << std::endl;
			}
			
			return false;
		}

        return true;
    }
    
    
    //-----------------------------------------------------------------------------
    Poco::Optional<ProtocolInfo> HttpCommandInterface::getProtocolInfo()
    {
        // Read protocol info via HTTP/JSON request/response
        if( !sendHttpCommand("get_protocol_info") || !checkErrorCode() )
            return Poco::Optional<ProtocolInfo>();
        
        // Read and set protocol info
		Json::Value protocol_name = root["protocol_name"];
		Json::Value version_major = root["version_major"];
		Json::Value version_minor = root["version_minor"];
		Json::Value ocommands = root["commands"];

		if (!protocol_name.isString() || !version_major.isNumeric() || !version_minor.isNumeric() || ocommands.isNull())
			return Poco::Optional<ProtocolInfo>();
		
        
        ProtocolInfo pi;
		pi.protocol_name = protocol_name.asString();
		pi.version_major = version_major.asInt();
		pi.version_minor = version_minor.asInt();

		
		if (ocommands.isArray()) {
			for (int i=0; i<ocommands.size(); i++) {
				Json::Value v = ocommands[i];
				
				try {
					pi.commands.push_back(v.asString());
				} catch(Exception e) {
					std::cerr << e.displayText();
				}
			}
		}

        return pi;
    }

    
    //-----------------------------------------------------------------------------
    std::vector< std::string > HttpCommandInterface::getParameterList()
    {
        // Read available parameters via HTTP/JSON request/response
        std::vector< std::string > parameter_list;
        if( !sendHttpCommand("list_parameters") || !checkErrorCode() )
            return parameter_list;
		
		Json::Value oparameters = root["parameters"];
		
		if (oparameters.isNull()) {
			return parameter_list;
		}
		
		if (oparameters.isArray()) {
			for (int i=0; i<oparameters.size(); i++) {
				Json::Value v = oparameters[i];
				try {
					parameter_list.push_back(v.asString());
				} catch(Exception e) {
					std::cerr << e.displayText();
				}
			}
		}

        return parameter_list;
    }

    
    
    //-----------------------------------------------------------------------------
    Poco::Optional<HandleInfo> HttpCommandInterface::requestHandleTCP(int start_angle)
    {
        // Prepare HTTP request
        std::map< std::string, std::string > params;
        params["packet_type"] = "C";
        params["start_angle"] = Poco::NumberFormatter::format(start_angle);
        
        // Request handle via HTTP/JSON request/response
        if( !sendHttpCommand("request_handle_tcp", params) || !checkErrorCode() )
            return Poco::Optional<HandleInfo>();

		Json::Value port = root["port"];
		Json::Value handle = root["handle"];
		
		if(!port.isInt() || !handle.isString())
			return Poco::Optional<HandleInfo>();

		
        // Prepare return value
        HandleInfo hi;
        hi.handle_type = HandleInfo::HANDLE_TYPE_TCP;
		hi.handle = handle.asString();
        hi.hostname = http_host_;
		hi.port = port.asInt();
        hi.packet_type = 'C';
        hi.start_angle = start_angle;
        hi.watchdog_enabled = true;
        hi.watchdog_timeout = 60000;
        return hi;
    }
    
    
    
    //-----------------------------------------------------------------------------
    Poco::Optional<HandleInfo> HttpCommandInterface::requestHandleUDP(int port, std::string hostname, int start_angle)
    {
        // Prepare HTTP request
        if( hostname == "" )
            hostname = discoverLocalIP();
        std::map< std::string, std::string > params;
        params["packet_type"] = "C";
        params["start_angle"] = Poco::NumberFormatter::format(start_angle);
        params["port"] = Poco::NumberFormatter::format(port);
        params["address"] = hostname;
        
        // Request handle via HTTP/JSON request/response
        if( !sendHttpCommand("request_handle_udp", params) || !checkErrorCode() )
            return Poco::Optional<HandleInfo>();
		
		Json::Value handle = root["handle"];
		if(!handle.isString())
			return Poco::Optional<HandleInfo>();
		
		
        // Prepare return value
        HandleInfo hi;
        hi.handle_type = HandleInfo::HANDLE_TYPE_UDP;
		hi.handle = handle.asString();
        hi.hostname = hostname;
        hi.port = port;
        hi.packet_type = 'C';
        hi.start_angle = start_angle;
        hi.watchdog_enabled = true;
        hi.watchdog_timeout = 60000;
        return hi;
    }
    
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::releaseHandle(const std::string& handle)
    {
        if( !sendHttpCommand("release_handle", "handle", handle) || !checkErrorCode() )
            return false;
        return true;
    }
    
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::startScanOutput(const std::string& handle)
    {
        if( !sendHttpCommand("start_scanoutput", "handle", handle) || !checkErrorCode() )
            return false;
        return true;
    }
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::stopScanOutput(const std::string& handle)
    {
        if( !sendHttpCommand("stop_scanoutput", "handle", handle) || !checkErrorCode() )
            return false;
        return true;
    }
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::feedWatchdog(const std::string &handle)
    {
        if( !sendHttpCommand("feed_watchdog", "handle", handle) || !checkErrorCode() )
            return false;
        return true;
    }
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::rebootDevice()
    {
        if( !sendHttpCommand("reboot_device") || !checkErrorCode() )
            return false;
        return true;
    }
	
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::resetParameters(const std::vector<std::string> &names)
    {
        // Prepare HTTP request
        std::string namelist;
        
        for( std::vector<std::string>::const_iterator s = names.begin(); s != names.end(); s++ )
            namelist += (*s + ";");
        namelist.substr(0,namelist.size()-1);
        
        if( !sendHttpCommand("reset_parameter","list",namelist) || ! checkErrorCode()  )
            return false;
        
        return true;
    }
	
    //-----------------------------------------------------------------------------
    std::string HttpCommandInterface::discoverLocalIP()
    {
        std::string local_ip;
        try
        {
			SocketAddress sa(http_host_, http_port_);
			DatagramSocket sock;
			sock.connect(sa);
			
			// get host address
			SocketAddress a = sock.address();
			local_ip = a.host().toString();
			
			sock.close();
        }
        catch (std::exception& e)
        {
            std::cerr << "Could not deal with socket-exception: " << e.what() << std::endl;
        }
        
        return local_ip;
    }
}