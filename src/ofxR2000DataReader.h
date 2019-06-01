//
//  R2000DataReader.hpp
//  R2000DataReader
//
// written by Ingo Randolf, 2015
// https://github.com/i-n-g-o/ofxR2000
//
//
//

#ifndef R2000DataReader_h
#define R2000DataReader_h

#include "ofMain.h"
#include "ofThread.h"

#include "ofxR2000.h"

using namespace pepperl_fuchs;

class R2000DataReader : public ofThread
{
	
public:
	R2000DataReader();
	R2000DataReader(string& filepath);
	R2000DataReader(ofFile& file);
	~R2000DataReader();
	
	bool load(const char* filepath);
	bool load(string& filepath);
	void load(ofFile& file);
	bool isLoaded() { return isOpen; };
	
	int getSamplesPerScan() { return samplesPerScan; };
	int getScanFrequency() { return scanFrequency; };
	uint64_t getCount() { return count; };
	
	ScanData getScan();
	std::size_t getScansAvailable();
	std::size_t getFullScansAvailable();
	
	
private:
	void threadedFunction();
	void initRead();
	void getNextScan();
	
	ofFile infile;
	bool isOpen;
	
	int dataStart;
	uint64_t count;
	
	int samplesPerScan, scanFrequency;
	
	std::deque<ScanData> scan_data_;
	
//	ScanData lastScanData;
	
	double updateTime; // [ms]
	uint64_t lastUpdate;
};

#endif /* R2000DataReader_hpp */
