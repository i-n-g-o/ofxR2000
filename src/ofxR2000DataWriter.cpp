//
//  ofxR2000DataWriter.cpp
//  r2000DataReader
//
//  Created by inx on 21/12/15.
//
//
#include <zlib.h>

#include "ofxR2000DataWriter.h"


static std::vector< unsigned char >
zipcompress( const std::vector< std::uint32_t > & src, int level )
{
	std::vector< unsigned char > ret;
	
	uLongf ret_size = ::compressBound( src.size() * sizeof(std::uint32_t) );
	ret.resize( ret_size + sizeof( uLongf ) );
	
	
	if ( level == 0 )
	{
		int error = ::compress( ret.data() + sizeof( uLongf ), &ret_size, (unsigned char*)src.data(), src.size()*sizeof(std::uint32_t) );
		if ( error == Z_OK )
		{
			ret.resize( ret_size + sizeof( uLongf ), 0 );
		}
		else
		{
			ofLogError( "ofxZip::compress()" ) << "zlib compress() error: " << error;
			ret.clear();
		}
	}
	else
	{
		int error = compress2( ret.data() + sizeof( uLongf ), &ret_size, (unsigned char*)src.data(), src.size()*sizeof(std::uint32_t), level );
		if ( error == Z_OK )
		{
			ret.resize( ret_size + sizeof( uLongf ), 0 );
		}
		else
		{
			ofLogError( "ofxZip::compress()" ) << "zlib compress2() error: " << error;
			ret.clear();
		}
	}
	
	/// push header (size of compressed buffer)
	{
		uLongf originalSize = src.size();
		ret[0] = ( originalSize >> 24 ) & 0xFF;
		ret[1] = ( originalSize >> 16 ) & 0xFF;
		ret[2] = ( originalSize >>  8 ) & 0xFF;
		ret[3] = ( originalSize >>  0 ) & 0xFF;
		
	}
	
	return ret;
}

R2000DataWriter::R2000DataWriter() :
	counter(0)
	,doCompress(true)
	,bisInit(false)
{}

R2000DataWriter::R2000DataWriter(string& filepath) : R2000DataWriter()
{
	openFile(filepath);
}

R2000DataWriter::R2000DataWriter(string& filepath, uint32_t samplesPerScan, uint32_t scanFrequency) : R2000DataWriter(filepath)
{
	init(samplesPerScan, scanFrequency);
}

R2000DataWriter::~R2000DataWriter() {
	closeFile();
}


bool R2000DataWriter::openFile(string filepath) {
	
	// we will open file in append-mode
	// remove file if it exists
	ofFile tmp(filepath);
	if (tmp.exists() && tmp.isFile()) {
		tmp.remove();
	}
	tmp.close();
	
	bFileOpen = file.open(filepath, ofFile::Append, true);
	return bFileOpen;
}

void R2000DataWriter::closeFile() {
	
	file.close();
	bFileOpen = false;
	bisInit = false;
	
}

void R2000DataWriter::init(uint32_t samplesPerScan, uint32_t scanFrequency) {
	
	if (!bFileOpen) {
		return;
	}
	
	uint8_t sizeofint32 = sizeof(std::uint32_t);
	
	// dump header, and info
	ofBuffer dataBuffer;
	// size of int
	dataBuffer.append((char*)&sizeofint32, sizeof(uint8_t));
	// samples per scan
	dataBuffer.append((char*)&samplesPerScan, sizeofint32);
	// size of int
	dataBuffer.append((char*)&sizeofint32, sizeof(uint8_t));
	// scan frequency
	dataBuffer.append((char*)&scanFrequency, sizeofint32);
	
	// write to file
	file.writeFromBuffer(dataBuffer);
	
	bisInit = true;
}

void R2000DataWriter::writeScanData(ScanData& data) {
	
	if (!bisInit) {
		ofLogError() << "ScanDataWriter is not inited. Please call init(...) before writing ScanData.";
		return;
	}
	
	std::uint32_t* dd = data.distance_data.data();
	std::uint32_t* da = data.amplitude_data.data();
	PacketHeader* headers = data.headers.data();
	
	uint8_t sizeofint64 = sizeof(std::uint64_t);
	uint8_t sizeouint32 = sizeof(std::uint32_t);
	
	ofBuffer dataBuffer;
	
	// counter
	dataBuffer.append((char*)&sizeofint64, sizeof(uint8_t));
	dataBuffer.append((char*)&counter, sizeofint64);
	
	
	
	
	// distance
	// compression-flag
	dataBuffer.append((char*)&doCompress, 1);
	if (doCompress) {
		vector< unsigned char > compressed = zipcompress( data.distance_data, 0 );
		std::uint32_t vectorSize = compressed.size();
		dataBuffer.append((char*)&sizeouint32, sizeof(uint8_t));
		dataBuffer.append((char*)&vectorSize, sizeouint32);
		dataBuffer.append((const char *)compressed.data(), vectorSize);
	} else {
		// uncompressed
		std::uint32_t vectorSize = data.distance_data.size();
		dataBuffer.append((char*)&sizeouint32, sizeof(uint8_t));
		dataBuffer.append((char*)&vectorSize, sizeouint32);
		dataBuffer.append((const char *)dd, vectorSize*sizeof(std::uint32_t));
	}
	
	// amp size of elements
	// compression-flag
	dataBuffer.append((char*)&doCompress, 1);
	if (doCompress) {
		vector< unsigned char > compressed = zipcompress( data.amplitude_data, 0 );
		std::uint32_t vectorSize = compressed.size();
		dataBuffer.append((char*)&sizeouint32, sizeof(uint8_t));
		dataBuffer.append((char*)&vectorSize, sizeof(std::uint32_t));
		dataBuffer.append((const char *)compressed.data(), vectorSize);
	} else {
		// uncompressed
		std::uint32_t vectorSize = data.amplitude_data.size();
		dataBuffer.append((char*)&sizeouint32, sizeof(uint8_t));
		dataBuffer.append((char*)&vectorSize, sizeouint32);
		dataBuffer.append((const char *)da, vectorSize*sizeof(std::uint32_t));
	}
	
	// headers
	// compression-flag - don't compress headers for now
	bool bFalse = false;
	dataBuffer.append((char*)&bFalse, 1);
	if (doCompress || !doCompress) {
		std::uint32_t  vectorSize = data.headers.size();
		dataBuffer.append((char*)&sizeouint32, sizeof(uint8_t));
		dataBuffer.append((char*)&vectorSize, sizeouint32);
		dataBuffer.append((const char *)headers, vectorSize*sizeof(PacketHeader));
	}
	
	file.writeFromBuffer(dataBuffer);
	
	counter++;
}