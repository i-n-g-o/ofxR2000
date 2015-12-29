//
//  ofxR2000DataWriter.hpp
//  r2000DataReader
//
//  Created by inx on 21/12/15.
//
//

#ifndef ofxR2000DataWriter_hpp
#define ofxR2000DataWriter_hpp

#include "ofMain.h"
#include "ofxR2000.h"

using namespace pepperl_fuchs;

class R2000DataWriter {
	
public:
	R2000DataWriter();
	R2000DataWriter(string& filepath);
	R2000DataWriter(string& filepath, uint32_t samplesPerScan, uint32_t scanFrequency);
	~R2000DataWriter();
	
	bool openFile(string filepath);
	void closeFile();
	bool isOpen() { return bFileOpen; };
	
	void init(uint32_t samplesPerScan, uint32_t scanFrequency);
	void writeScanData(ScanData& data);
	
	void setCompress(bool val) { doCompress = val; };
	bool getCompress() const { return doCompress; };
	
	uint64_t getCount() const { return counter; };
	uint64_t getSize() const { return file.getSize(); };
	
	
private:
	ofFile file;
	uint64_t counter;
	bool doCompress;
	
	bool bFileOpen;
	bool bisInit;
};

#endif /* ofxR2000DataWriter_hpp */
