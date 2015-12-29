#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFrameRate(60);
	
//	samplesPerScan		= 25200;
//	scanFrequency		= 10;
	
//	samplesPerScan		= 5040;
//	scanFrequency		= 50;

	samplesPerScan		= 3600;
	scanFrequency		= 49;

	//----------------------------------------
	// generate a box vboMesh from a primitive.
	mesh = ofMesh::box(1,1,1,1,1,1);
	mesh.setUsage(GL_STATIC_DRAW);
	
	
	//------------------------------------------------
	// init vars
	//------------------------------------------------
	lastScanData = pepperl_fuchs::ScanData();

	
	//------------------------------------------------
	// R2000
	//------------------------------------------------
	lastSampleValid = false;
	scanner_ip = "10.0.10.9";
	//    scanner_ip = "192.168.178.113";
	
	ofLogNotice() << "============================================================";
	ofLogNotice() << " R2000";
	ofLogNotice();
	
	string info = "Connecting to scanner at ";
	info.append(scanner_ip);
	info.append(" ... ");
	
	if (driver.connect(scanner_ip, 80)) {
		ofLogNotice() << info << "OK";
	} else {
		ofLogNotice() << info << "FAILED!";
		ofLogError() << "Connection to scanner at " << scanner_ip << " failed!";
		
		OF_EXIT_APP(-1);
	}
	
	
	
	
	// set direction of scan
	if (!driver.setParameter("scan_direction", "cw")) {
		ofLogNotice() << "error setting direction";
	}
	
	// set indicator
	driver.setParameter("locator_indication", "off");
	driver.setParameter("hmi_display_mode", "bargraph_distance"); //off, static_logo, bargraph_distance, bargraph_echo, bargraph_reflector
	
	
	// set lowest
	driver.setScanFrequency(10);
	driver.setSamplesPerScan(72);
	
	
	// set scan freqeuecy, and samples per scan
	driver.setScanFrequency(scanFrequency);
	driver.setSamplesPerScan(samplesPerScan);
	
	
	
	ofLogNotice() << "\t\t===================================================";
	ofLogNotice() << "setup R2000";
	ofLogNotice() << "scanFrequency: " << scanFrequency;
	ofLogNotice() << "samplesPerScan: " << samplesPerScan;
	ofLogNotice();
	
	
	
	// print out scanner parameters
	map<string, string> params = driver.getParameters();
	ofLogNotice() << "Current scanner settings:";
	ofLogNotice() << "============================================================";
	
	for (map<string, string>::iterator iterator = params.begin(); iterator != params.end(); iterator++) {
		ofLogNotice() << iterator->first << " : " << iterator->second;
	}
	ofLogNotice() << "============================================================";
	
	
	
	//-------------------------------------------------------------------------
	// Start capturing scanner data
	//-------------------------------------------------------------------------
	info = "Starting capturing: ";
	if (driver.startCapturingUDP()) {
		ofLogNotice() << info << "OK";
	} else {
		ofLogNotice() << info << "FAILED!";
		
		OF_EXIT_APP(-1);
	}

	
	//-------------------------------------------------------------------------
	// file
	//-------------------------------------------------------------------------
	string filename = ofGetTimestampString("%Y_%m_%d-%H_%M_%S-r2000.bin");
	ofLogNotice() << "opening file: " << filename;
	
	// setup dataWriter
	dataWriter.setCompress(true);
	bool succ = dataWriter.openFile(ofToDataPath(filename));
	if (succ) {
		// init writer, write header
		dataWriter.init(samplesPerScan, scanFrequency);
	} else {
		ofLogError() << "error opening file: " << filename;
	}
	
	
	//-------------------------------------------------------------------------
	// init shader
	//-------------------------------------------------------------------------
	shader.load("shader/vert.glsl", "shader/frag.glsl");
	
	
	minutes = ofGetMinutes();
}

//--------------------------------------------------------------
void ofApp::update(){

	// get available scans
	scans_available = driver.getFullScansAvailable();
	
	if (scans_available > 0) {
		
		// lock scan data mutex
		ofScopedLock lock(scanDataMutex);
		
		ScanData data;
		
		lastSampleValid = true;
		
		// get all the scans
		for (size_t i = 0; i < scans_available; i++)
		{
			data = driver.getScan();
		}
		
		//----------------------------------------
		// write data
		uint64_t now = ofGetElapsedTimeMicros();
		dataWriter.writeScanData(data);
		writtenIn = ((ofGetElapsedTimeMicros() - now)/1000000.0);
		
		
		
		//----------------------------------------
		// update distance texture
		if (!pixelBufferDist.isAllocated() ||
			pixelBufferDist.size() != (lastScanData.distance_data.size() * sizeof(std::uint32_t)))
		{
			ofLogNotice() << "allocate textureBuffer: " << lastScanData.distance_data.size();
			
			pixelBufferDist.allocate();
			pixelBufferDist.bind(GL_TEXTURE_BUFFER);
			pixelBufferDist.setData(lastScanData.distance_data, GL_STREAM_DRAW);
			
			texDist.allocateAsBufferTexture(pixelBufferDist, GL_RGBA8);
			
			shader.begin();
			shader.setUniformTexture("texDist", texDist, shader.getUniformLocation("texDist"));
			shader.setUniform1f("size", lastScanData.distance_data.size());
			shader.end();
		} else {
			pixelBufferDist.updateData(0, lastScanData.distance_data);
		}
		
		
		//----------------------------------------
		// update amplitude texture
		if (!pixelBufferAmp.isAllocated() ||
			pixelBufferAmp.size() != (lastScanData.amplitude_data.size() * sizeof(std::uint32_t)))
		{
			ofLogNotice() << "resize amp texture: " << lastScanData.amplitude_data.size();
			
			pixelBufferAmp.allocate();
			pixelBufferAmp.bind(GL_TEXTURE_BUFFER);
			pixelBufferAmp.setData(lastScanData.amplitude_data, GL_STREAM_DRAW);
			
			texAmp.allocateAsBufferTexture(pixelBufferAmp, GL_RGBA8);
			
			shader.begin();
			shader.setUniformTexture("texAmp", texAmp, shader.getUniformLocation("texAmp"));
			shader.end();
		} else {
			pixelBufferAmp.updateData(0, lastScanData.amplitude_data);
		}
		
		
		
		lastScanData = data;
	}
	
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(40);
	
	cam.begin();
	
	ofFill();
	ofSetColor(0, 180, 0);
	ofDrawCircle(0, 0, 1);
	
	if (lastSampleValid) {
		// draw boxes
		shader.begin();
		mesh.drawInstanced(OF_MESH_FILL, lastScanData.distance_data.size());
		shader.end();
	}
	
	cam.end();
	
	ofSetColor(220);
	ofDrawBitmapString("count: " + ofToString(dataWriter.getCount()), 20, 20);
	ofDrawBitmapString("size: " + ofToString((dataWriter.getSize()/1048576.0)) + " MB", 20, 40);
	ofDrawBitmapString("written in: " + ofToString(writtenIn) + " sec", 20, 60);
	ofDrawBitmapString(ofToString(ofGetFrameRate()) + " fps", 20, 80);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

void ofApp::exit()
{
	ofLogNotice() << "exit";
	
	if (driver.isConnected()) {
		driver.setScanFrequency(10);
		driver.setSamplesPerScan(72);
		driver.disconnect();
	}
	
	dataWriter.closeFile();
}
