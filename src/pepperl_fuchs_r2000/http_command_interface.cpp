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

#include "Poco/JSON/Query.h"
#include "Poco/JSON/JSONException.h"


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
        try
        {
            // reset parser
            jsonParser.reset();
            
            // parse json
            std::stringstream ss(content);
            jsonResult = jsonParser.parse(ss);
        }
        catch (Poco::JSON::JSONException& e)
        {
            std::cerr << "ERROR: Exception: " <<  e.what() << " (" << e.message() << ")" << std::endl;
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
        Poco::JSON::Query query(jsonResult);
        Poco::Dynamic::Var value = query.find(name);
        
        if (value.isEmpty())
            return Poco::Optional<std::string>();
        
        std::string s = Poco::Dynamic::Var::toString(value);
        
        return Poco::Optional<std::string>(s);
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
        
        
        Poco::JSON::Query query(jsonResult);
        
       
        // Extract values from JSON property_tree
        for( std::vector<std::string>::const_iterator s = names.begin(); s != names.end(); s++ )
        {
            Poco::Dynamic::Var value = query.find(*s);
            
            if( !value.isEmpty() )
                key_values[*s] = Poco::Dynamic::Var::toString(value);
            else
                key_values[*s] = "--COULD NOT RETRIEVE VALUE--";
        }
        
        return key_values;
    }
    
    
    //-----------------------------------------------------------------------------
    bool HttpCommandInterface::checkErrorCode()
    {
        Poco::JSON::Query query(jsonResult);
        Poco::Dynamic::Var error_value = query.find("error_code");
        Poco::Dynamic::Var error_string = query.find("error_text");
        
        if( !error_value.isNumeric() || (error_value.extract<int>()) != 0 || !error_string.isString() || (error_string.extract<std::string>()) != "success" )
        {
            if( error_string.isString() )
                std::cerr << "ERROR: scanner replied: " << error_string.extract<std::string>() << std::endl;
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
        Poco::JSON::Query query(jsonResult);
		
        Poco::Dynamic::Var protocol_name = query.find("protocol_name");
        Poco::Dynamic::Var version_major = query.find("version_major");
        Poco::Dynamic::Var version_minor = query.find("version_minor");
        Poco::Dynamic::Var ocommands = query.find("commands");
        
        if( !protocol_name.isString() || !version_major.isInteger() || !version_minor.isInteger() || ocommands.isEmpty() )
            return Poco::Optional<ProtocolInfo>();

        
        ProtocolInfo pi;
        pi.protocol_name = protocol_name.extract<std::string>();
        pi.version_major = version_major.extract<int>();
        pi.version_minor = version_minor.extract<int>();

        
        Poco::JSON::Array::Ptr arr = ocommands.extract<Poco::JSON::Array::Ptr>();
		
        for (Poco::JSON::Array::ValueVec::const_iterator i= arr->begin(); i!=arr->end(); i++) {
            std::string cmd = i->extract<std::string>();
            pi.commands.push_back(cmd);
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
        
        
        Poco::JSON::Query query(jsonResult);
        
        // Check if JSON contains the key "parameters"
        Poco::Dynamic::Var oparameters = query.find("parameters");
        if( !oparameters )
            return parameter_list;
        
        Poco::JSON::Array::Ptr arr = oparameters.extract<Poco::JSON::Array::Ptr>();
        
        for (Poco::JSON::Array::ValueVec::const_iterator i= arr->begin(); i!=arr->end(); i++) {
            std::string param = i->extract<std::string>();
            parameter_list.push_back(param);
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
        
        Poco::JSON::Query query(jsonResult);
        
        // Extract handle info from JSON response
        Poco::Dynamic::Var port = query.find("port");
        Poco::Dynamic::Var handle = query.find("handle");
        
        if(!port.isInteger() || !handle.isString())
            return Poco::Optional<HandleInfo>();
        
        // Prepare return value
        HandleInfo hi;
        hi.handle_type = HandleInfo::HANDLE_TYPE_TCP;
        hi.handle = handle.extract<std::string>();
        hi.hostname = http_host_;
        hi.port = port.extract<int>();
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
        
        Poco::JSON::Query query(jsonResult);
        
        // Extract handle info from JSON response
        Poco::Dynamic::Var handle = query.find("handle");
        if(!handle.isString())
            return Poco::Optional<HandleInfo>();
        
        // Prepare return value
        HandleInfo hi;
        hi.handle_type = HandleInfo::HANDLE_TYPE_UDP;
        hi.handle = handle.extract<std::string>();
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