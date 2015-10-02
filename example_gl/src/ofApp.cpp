#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){
	
	ofSetFrameRate(60);
	
	// init vars
	isScanning = false;
	lastSampleValid = false;
	
	string scanner_ip = "10.0.10.9";
	
	
	//----------------------------------------
	// generate a box vboMesh from a primitive.
	
	mesh = ofMesh::box(1,1,1,1,1,1);
	mesh.setUsage(GL_STATIC_DRAW);
	
	
	
	ofLogNotice() << "Connecting to scanner at " << scanner_ip << " ... ";
	if (driver.connect(scanner_ip, 80))
		ofLogNotice() << "OK";
	else
	{
		ofLogNotice() << "FAILED!";
		ofLogError() << "Connection to scanner at " << scanner_ip << " failed!";
		return;
	}
	
	if (!driver.setParameter("scan_direction", "cw")) {
		ofLogNotice() << "error setting direction";
	}
	isCw = true;
	
	
	driver.setParameter("locator_indication", "off");
	driver.setParameter("hmi_display_mode", "bargraph_distance"); //off, static_logo, bargraph_distance, bargraph_echo, bargraph_reflector
	
	
	//-------------------------------------------------------------------------
	// samples per scan
	/*
	 25 200		10 Hz
	 16 800		15 Hz
	 12 600		20 Hz
	 10 080		25 Hz
	 8400		30 Hz
	 7200		35 Hz
	 6300		40 Hz
	 5600		45 Hz
	 5040		50 Hz
	 4200		50 Hz
	 3600		50 Hz
	 2400		50 Hz
	 1800		50 Hz
	 1440		50 Hz
	 1200		50 Hz
	 900		50 Hz
	 800		50 Hz
	 720		50 Hz
	 600		50 Hz
	 480		50 Hz
	 450		50 Hz
	 400		50 Hz
	 360		50 Hz
	 240		50 Hz
	 180		50 Hz
	 144		50 Hz
	 120		50 Hz
	 90			50 Hz
	 72			50 Hz
	 */
	
	int samples = 5040;
	
	driver.setScanFrequency(50);
	driver.setSamplesPerScan(samples);
	
	angleStep = 360.0 / (float)samples;
	if (isCw) angleStep = -angleStep;
	
	const std::map< std::string, std::string >& params = driver.getParameters();
	
	ofLogNotice() << "Current scanner settings:";
	ofLogNotice() << "============================================================";
	
	
	for(std::map< std::string, std::string >::const_iterator p = params.begin(); p != params.end(); p++) {
		ofLogNotice() << p->first << " : " << p->second;
	}
	
	ofLogNotice() << "============================================================";
	
	
	// Start capturing scanner data
	//-------------------------------------------------------------------------
	if (driver.startCapturingUDP()) {
		//	if (driver.startCapturingTCP()) {
		isScanning = true;
	} else
	{
		ofLogNotice() << "start capture failed!";
		isScanning = false;
	}
	
	
	// init shader
	shader.load("vert.glsl","frag.glsl");
}

//--------------------------------------------------------------
void ofApp::update(){
	if (isScanning) {
		// get available scans
		size_t scans_available = driver.getFullScansAvailable();
		
		if (scans_available > 0) {
			for (int i = 0; i < scans_available; i++) {
				lastScanData = driver.getScan();
			}
			lastSampleValid = true;
			
			
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
		}
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
	
	ofSetColor(180);
	string info = "fps: " + ofToString(ofGetFrameRate());
	ofDrawBitmapString(info, 32, 32);
	
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

//--------------------------------------------------------------
void ofApp::exit()
{
	driver.setScanFrequency(10);
	driver.disconnect();
}