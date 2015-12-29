#pragma once

#include "ofMain.h"

#include "ofxR2000.h"

using namespace pepperl_fuchs;

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void exit();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
	
	
	// scanner
	std::string scanner_ip;
	R2000Driver driver;

	
	ScanData lastScanData;
	std::deque<uint32_t> scan_data;
	std::deque<uint32_t> scan_data_idx;
	std::deque<uint32_t> scan_data_amp;
	
	int samplesPerScan;
	int scanFrequency;
	
	
	bool lastSampleValid;
	ofMutex scanDataMutex;
	
	size_t scans_available;
	
	
	
	// dump data
	float writtenIn;
	int minutes;
	R2000DataWriter dataWriter;
	
	
	// drawing
	ofEasyCam	cam;
	ofVboMesh	mesh;
	ofShader	shader;
	
	ofBufferObject pixelBufferDist;
	ofBufferObject pixelBufferAmp;
	
	ofTexture texDist;
	ofTexture texAmp;
};
