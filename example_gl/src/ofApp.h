#pragma once

#include "ofMain.h"

#include "ofxR2000.h"

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
	
	
	pepperl_fuchs::R2000Driver driver;
	pepperl_fuchs::ScanData lastScanData;
	bool isScanning;
	bool isCw;
	bool lastSampleValid;
	
	double angleStep;
	
	ofEasyCam cam;

	// drawing
	ofVboMesh	mesh;
	ofShader	shader;

	ofBufferObject pixelBufferDist;
	ofBufferObject pixelBufferAmp;
	
	ofTexture texDist;
	ofTexture texAmp;
	
};