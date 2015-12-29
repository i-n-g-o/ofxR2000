#include "ofApp.h"

int sps[] = {25200, 16800, 12600, 10080, 8400, 7200, 6300, 5600, 5040, 4200, 3600, 2400, 1800, 1440, 1200, 900, 800, 720, 600, 480, 450, 400, 360, 240, 180, 144, 120, 90, 72};
int sps_size = sizeof(sps) / sizeof(int);

//--------------------------------------------------------------
void ofApp::setup(){

	
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();
	ofBackground(40);
	
	
	bool succ = dataReader.load("/Users/inx/Documents/_src/openFrameworks_orig/apps/myApps/r2000DataRecorder/bin/data/25200_10/2015_10_04-13_28_24-r2000-empty-room.bin");
	
	
	angleStep = 360.0 / (float)dataReader.getSamplesPerScan();
//	if (isCw) angleStep = -angleStep;

	ofLogNotice() << "samplesPerScan: " << dataReader.getSamplesPerScan();
	ofLogNotice() << "scanFrequency: " << dataReader.getScanFrequency();

	
	
	//----------------------------------------
	// generate a box vboMesh from a primitive.
	mesh = ofMesh::box(1,1,1,1,1,1);
	mesh.setUsage(GL_STATIC_DRAW);
	
	//----------------------------------------
	// load shader
	shader.load("shader/vert.glsl","shader/frag.glsl");
	
	GLint err = glGetError();
	if (err != GL_NO_ERROR){
		ofLogNotice() << "Load Shader came back with GL error:	" << err;
	}
}

//--------------------------------------------------------------
void ofApp::update(){

	if (dataReader.update()) {
		lastScanData = dataReader.getLastScanData();
		
		//----------------------------------------------------
		// distance data
		if (!pixelBufferDist.isAllocated() ||
			pixelBufferDist.size() != (lastScanData.distance_data.size() * sizeof(std::uint32_t)))
		{
//			ofLogNotice() << "allocate textureBuffer: " << lastScanData.distance_data.size();
			
			pixelBufferDist.allocate();
			pixelBufferDist.setData(lastScanData.distance_data, GL_STREAM_DRAW);
			
			texDist.allocateAsBufferTexture(pixelBufferDist, GL_RGBA8);
			//			texDist.allocateAsBufferTexture(pixelBufferDist, GL_R32UI);
			
			
			shader.begin();
			
			shader.setUniformTexture("texDist", texDist, 1);
			shader.setUniform1f("size", lastScanData.distance_data.size());
			shader.end();
		} else {
			//			pixelBufferDist.bind(GL_TEXTURE_BUFFER);
			pixelBufferDist.updateData(0, lastScanData.distance_data);
		}

		
		//----------------------------------------------------
		// amplitude data
		if (!pixelBufferAmp.isAllocated() ||
			pixelBufferAmp.size() != (lastScanData.distance_data.size() * sizeof(std::uint32_t)))
		{
//			ofLogNotice() << "resize amp texture";
			
			pixelBufferAmp.allocate();
			pixelBufferAmp.setData(lastScanData.amplitude_data, GL_STREAM_DRAW);
			
			texAmp.allocateAsBufferTexture(pixelBufferAmp, GL_RGBA8);
			
			shader.begin();
			shader.setUniformTexture("texAmp", texAmp, 0);
			shader.end();
		} else {
			//			pixelBufferAmp.bind(GL_TEXTURE_BUFFER);
			pixelBufferAmp.updateData(0, lastScanData.amplitude_data);
		}

	}
}

//--------------------------------------------------------------
void ofApp::draw(){

	cam.begin();
	
	// draw center
	ofFill();
	ofSetColor(0, 180, 0);
	ofDrawCircle(0, 0, 1);

	// draw boxes
	shader.begin();
	
	shader.setUniformTexture("texDist", texDist, 1);
	shader.setUniformTexture("texAmp", texAmp, 0);
	
	
	mesh.drawInstanced(OF_MESH_FILL, lastScanData.distance_data.size());
	shader.end();
	
	cam.end();
	
	
	// draw text
	ofSetColor(180);
	string info = "fps: " + ofToString(ofGetFrameRate());
	ofDrawBitmapString(info, 32, 32);
	
	info = "count: " + ofToString(dataReader.getCount());
	ofDrawBitmapString(info, 32, 64);
	
	info = "samples per scan: " + ofToString(dataReader.getSamplesPerScan());
	ofDrawBitmapString(info, 32, 80);
	
	info = "frequency: " + ofToString(dataReader.getScanFrequency());
	ofDrawBitmapString(info, 32, 96);
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
void ofApp::exit(){
}
