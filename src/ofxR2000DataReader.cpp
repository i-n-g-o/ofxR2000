//
//  R2000DataReader.cpp
//  R2000DataReader
//
//  Created by inx on 20/12/15.
//
//

#include "ofxR2000DataReader.h"

#include <zlib.h>

static int sps[] = {25200, 16800, 12600, 10080, 8400, 7200, 6300, 5600, 5040, 4200, 3600, 2400, 1800, 1440, 1200, 900, 800, 720, 600, 480, 450, 400, 360, 240, 180, 144, 120, 90, 72};
static int sps_size = sizeof(sps) / sizeof(int);


/*
 */
static void zipuncompress_uint32( const std::vector< unsigned char > & src, std::vector< std::uint32_t > & ret )
{
	if ( src.size() < 4 ) {
		ret.clear();
		return;
	}
	
	/// load header (size of compressed buffer)
	uLongf originalSize = 0;
	{
		originalSize += ( src[0] << 24 );
		originalSize += ( src[1] << 16 );
		originalSize += ( src[2] <<  8 );
		originalSize += ( src[3] <<  0 );
	}
	
	ret.resize( originalSize, 0 );
	unsigned long ret_size = originalSize * sizeof(std::uint32_t);
	{
		// try to uncompress with sizeof( uLongf )
		int error = uncompress( (unsigned char*)ret.data(), &ret_size, src.data() + sizeof( uLongf ), src.size() );
		
		if ( error == Z_OK )
		{
			ret.resize( ret_size / sizeof(std::uint32_t), 0 );
		}
		else
		{
			// failed... try with other uLongSize...
			int error = Z_OK;
			
			if (sizeof(uLongf) == 4) {
				// try with uLongF size 8
				uint8_t sizeOfULongf = 8;
				error = uncompress( (unsigned char*)ret.data(), &ret_size, src.data() + sizeOfULongf, src.size() );
				
			} else if (sizeof(uLongf) == 8) {
				// try with uLongF size 4
				uint8_t sizeOfULongf = 4;
				error = uncompress( (unsigned char*)ret.data(), &ret_size, src.data() + sizeOfULongf, src.size() );
			}
			
			if ( error == Z_OK )
			{
				ret.resize( ret_size / sizeof(std::uint32_t), 0 );
			} else {
				ofLogError( "zipuncompress_uint32()" ) << "zlib uncompress() error: " << error;
				ret.clear();
			}
		}
	}
}


/*
 */
R2000DataReader::R2000DataReader() :
	dataStart(0)
	,count(0)
	,samplesPerScan(0)
	,scanFrequency(0)
	,updateTime(0)
	,lastUpdate(0)
{}

R2000DataReader::R2000DataReader(string& filepath) : R2000DataReader() {
	
	bool ret = load(filepath);
}


R2000DataReader::R2000DataReader(ofFile& file) : R2000DataReader() {
	load(file);
}

R2000DataReader::~R2000DataReader() {
	infile.close();
}


bool R2000DataReader::load(const char* filepath) {
	string p(filepath);
	return load(p);
}

bool R2000DataReader::load(string& filepath) {

	bool ret = infile.open(filepath);
	
	if (ret){
		initRead();
	}
	
	return ret;
}

void R2000DataReader::load(ofFile& file) {
	infile = file;
	initRead();
}


void R2000DataReader::initRead() {
	
	if (!infile.isFile()) {
		return;
	}
	
	uint8_t readSize;
	
	infile.read((char*)&readSize, sizeof(uint8_t));
	infile.read((char*)&samplesPerScan, readSize);
	
	infile.read((char*)&readSize, sizeof(uint8_t));
	infile.read((char*)&scanFrequency, readSize);
	
	//----------------------------------------
	// check values
	bool correctValue = false;
	for (int j=0; j<sps_size; j++) {
		if (samplesPerScan == sps[j]) {
			correctValue = true;
			break;
		}
	}
	
	if (scanFrequency < 10 || scanFrequency > 50) {
		correctValue = false;
	}
	
	
	if (!correctValue) {
		ofLogError() << "not a correct value";
		OF_EXIT_APP(-1);
	}

	lastScanData.distance_data.resize(samplesPerScan);
	lastScanData.amplitude_data.resize(samplesPerScan);
	
	//----------------------------------------
	updateTime = 1000.0 / (double)scanFrequency;
	dataStart = infile.tellg();
	lastUpdate = ofGetElapsedTimeMillis() - updateTime;
}


bool R2000DataReader::update() {
	
	bool newScan = false;
	
	if ((double)(ofGetElapsedTimeMillis() - lastUpdate) >= updateTime) {
		
		lastUpdate = ofGetElapsedTimeMillis();
		
		bool doCompress = false;
		
		uint8_t readSize;
		std::uint32_t vectorSize;
		
		//----------------------------------------------------
		// get count
		infile.read((char*)&readSize, sizeof(uint8_t));
		// check for eof, assume that data is alligned
		if (infile.eof()) {
			infile.clear();
			infile.seekg(dataStart);
			
			// read byte again
			infile.read((char*)&readSize, sizeof(uint8_t));
		}
		infile.read((char*)&count, readSize);
		
		
		//----------------------------------------------------
		// distance data
		
		// compression-flag
		infile.read((char*)&doCompress, 1);
		
		infile.read((char*)&readSize, sizeof(uint8_t));
		infile.read((char*)&vectorSize, readSize);
		if (doCompress) {
			std::vector<unsigned char> data;
			data.resize(vectorSize);
			infile.read((char*)data.data(), vectorSize);
			
			ofLogNotice() << "uncompress distance data: " << vectorSize;
			
			zipuncompress_uint32(data, lastScanData.distance_data);
			
			// correct
			vectorSize = lastScanData.distance_data.size();
		} else {
			if (lastScanData.distance_data.size() != vectorSize) {
				ofLogNotice() << "resize distance vector";
				lastScanData.distance_data.resize(vectorSize);
			}
			infile.read((char*)lastScanData.distance_data.data(), (vectorSize * sizeof(std::uint32_t)));
		}
		
		
		//----------------------------------------------------
		// amplitude data
		
		// compression-flag
		infile.read((char*)&doCompress, 1);
		
		infile.read((char*)&readSize, sizeof(uint8_t));
		infile.read((char*)&vectorSize, readSize);
		if (doCompress) {
			std::vector<unsigned char> data;
			data.resize(vectorSize);
			infile.read((char*)data.data(), vectorSize);
			
			ofLogNotice() << "uncompress amplitude data: " << vectorSize;
			
			zipuncompress_uint32(data, lastScanData.amplitude_data);
			// correct
			vectorSize = lastScanData.amplitude_data.size();
		} else {
			if (lastScanData.amplitude_data.size() != vectorSize) {
				ofLogNotice() << "resize amplitude vector";
				lastScanData.amplitude_data.resize(vectorSize);
			}
			infile.read((char*)lastScanData.amplitude_data.data(), (vectorSize * sizeof(std::uint32_t)));
		}
		
		
		//----------------------------------------------------
		// header data
		
		// compression-flag
		infile.read((char*)&doCompress, 1);
		
		infile.read((char*)&readSize, sizeof(uint8_t));
		infile.read((char*)&vectorSize, readSize);
		if (doCompress || !doCompress) {
			if (lastScanData.headers.size() != vectorSize) {
				lastScanData.headers.resize(vectorSize);
			}
			infile.read((char*)lastScanData.headers.data(), (vectorSize * sizeof(PacketHeader)));
		}
		//		ofLogNotice() << "HEADERS: " << vectorSize;
		
		newScan = true;
	}
	
	return newScan;
}