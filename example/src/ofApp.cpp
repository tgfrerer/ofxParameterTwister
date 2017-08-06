#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	// establish connection to midi device
	mTwister.setup();
	// set any parameters in paramsColor to be synced with 
	// the midi device
	mTwister.setParams(paramsColor);

	// setup our gui so we can have a look at the parameter values
	mPanel1.setup();
	mPanel1.add(paramsColor);

	// add event listener for pulse parameter
	mParamListeners.push_back(mParamPulseEncoders.newListener([this](bool & val) {
		for (int i = 0; i < 16; ++i) {
			if (val) {
				mTwister.setAnimationRotary(i, pal::Kontrol::ofxParameterTwister::Animation::PULSE, i % 4);
			} else {
				mTwister.setAnimationRotary(i, pal::Kontrol::ofxParameterTwister::Animation::NONE);
			}
		}
	}));


	mCam1.setupPerspective(false, 60, 0.1, 5000);
	mCam1.setPosition(ofVec3f(0, 0, 300));
	mCam1.lookAt({ 0, 0, 0 });

	mShdRender.load("shader");
	
	// disable TextureRect by default, use normalised texture coordinates
	ofDisableArbTex();

	ofLoadImage(mTexture, "sushi.jpg");

}

//--------------------------------------------------------------
void ofApp::update() {
	// This is where all currently mapped parameters get updated through midi.
	mTwister.update();
	// Place the above call before or after your other update code, 
	// depending at which point during execution you want midi parameters to 
	// affect your design

	if (mParamDemoMode) {
		float time = ofGetElapsedTimef();
		for (int j = 0; j < 4; ++j) {
			for (int i = 0; i < 4; ++i) {
				float val = fmod(time * .5f + (i + j) / 9.f, 1.f);
				mTwister.setHueRGB(j * 4 + i, val);
			}
		}
	}

}

//--------------------------------------------------------------
void ofApp::draw() {

	ofBackgroundGradient(ofColor::fromHex(0x323232), ofColor::black);

	ofEnableDepthTest();

	// draw some graphics based on the current parameters

	ofSetIcoSphereResolution(mParamResolution);

	mCam1.begin();
	{
		ofPushStyle();
		if (mParamUseAlpha) {
			ofEnableAlphaBlending();
		} else {
			ofDisableAlphaBlending();
		}


		ofSetColor(ofColor::fromHsb(mParamH, mParamS, mParamB, mParamA));
		if (mParamIsWireframe) {
			ofNoFill();
		} else {
			ofFill();
		}
		if (mShdRender.isLoaded()) {
			auto & shd = mShdRender;
			shd.begin();
			shd.setUniformTexture("src_texture_0", mTexture, 0);
			ofDrawIcoSphere(ofVec3f(), mParamSize);
			ofPushMatrix();
			ofDrawIcoSphere(ofVec3f(-200, 0, 0), mParamSize);
			ofDrawIcoSphere(ofVec3f(200,0,0), mParamSize);
			ofPopMatrix();
			shd.end();
		}
		
		ofPopStyle();
	}
	mCam1.end();

	ofDisableDepthTest();
	ofSetColor(ofColor::white);
	if (mParamShouldDrawInfo) {
		ofDrawBitmapString(
			"     _____    ___\n"
			"    /    /   /  /     ofxParameterTwister Example\n"
			"   /  __/ * /  /__    (c) ponies & light ltd., 2016. All rights reserved.\n"
			"  /__/     /_____/    poniesandlight.co.uk\n\n"
			"<i> toggle info text | <a|b> toggle parameter group", 10, ofGetHeight() - 12 * 7);

	}
	mPanel1.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

	switch (key)
	{
	case 'a':
		mPanel1.clear();
		mPanel1.add(paramsColor);
		mTwister.setParams(paramsColor);
		mParamDemoMode = false;
		break;
	case 'b':
		mPanel1.clear();
		mPanel1.add(paramsShape);
		mTwister.setParams(paramsShape);
		mParamDemoMode = false;
		break;
	case 'c':
		mPanel1.clear();
		mTwister.clear();
		mParamDemoMode = true;
		break;
	case 'i':
		mParamShouldDrawInfo ^= true;
		break;

	default:
		break;
	}

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
