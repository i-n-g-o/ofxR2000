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
#include "ofxR2000.h"

using namespace pepperl_fuchs;

class R2000DataReader {
	
public:
	R2000DataReader();
	R2000DataReader(string& filepath);
	R2000DataReader(ofFile& file);
	~R2000DataReader();
	
	bool load(const char* filepath);
	bool load(string& filepath);
	void load(ofFile& file);
	
	bool update();
	
	int getSamplesPerScan() { return samplesPerScan; };
	int getScanFrequency() { return scanFrequency; };
	ScanData& getLastScanData() { return lastScanData; };
	uint64_t getCount() { return count; };
	
	
private:
	void initRead();
	
	ofFile infile;
	int dataStart;
	uint64_t count;
	
	int samplesPerScan, scanFrequency;
	ScanData lastScanData;
	
	double updateTime; // [ms]
	uint64_t lastUpdate;
};

#endif /* R2000DataReader_hpp */
