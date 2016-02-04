#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	
	mKontrol1.setup();

	mPanel1.setup();

	mPanel1.add(mParamGroupA);
	mKontrol1.setParams(mParamGroupA);

}

//--------------------------------------------------------------
void ofApp::update(){
	mKontrol1.update(); // this is where all params get updated through midi.
}

//--------------------------------------------------------------
void ofApp::draw(){
	mPanel1.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	
	switch (key)
	{
	case 'a':
		mPanel1.clear();
		mPanel1.add(mParamGroupA);
		mKontrol1.setParams(mParamGroupA);
		break;
	case 'b':
		mPanel1.clear();
		mPanel1.add(mParamGroupB);
		mKontrol1.setParams(mParamGroupB);
		break;
	default:
		break;
	}

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
