#pragma once

#include "ofMain.h"
#include "ofxParameterTwister.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

	ofCamera mCam1;

	pal::Kontrol::ofxParameterTwister mTwister;

	ofParameter<float> 	mParamH{ "H", 0.f, 0.f, 255.f };
	ofParameter<float> 	mParamS{ "S", 0.f, 0.f, 255.f };
	ofParameter<float> 	mParamB{ "B", 255.f, 0.f, 255.f };
	ofParameter<float> 	mParamA{ "A", 255.f, 0.f, 255.f };
	ofParameter<bool> 	mParamUseAlpha{ "Use Alpha", true };
	ofParameter<bool> 	mParamPulseEncoders{ "Pulse Encoders", false };

	ofParameterGroup paramsColor{
		"Color",
		mParamH,
		mParamS,
		mParamB,
		mParamA,
		mParamUseAlpha,
		mParamPulseEncoders
	};

	ofParameter<float> 	mParamSize{ "Size", 75.f, 30.f, 400.f };
	ofParameter<float> 	mParamResolution{ "Resolution", 1.f, 0.5f, 4.f };
	ofParameter<bool> 	mParamIsWireframe{ "WireFrame", true };
	ofParameter<bool> 	mParamShouldDrawInfo{ "Toggle Info Text", true };

	ofParameterGroup paramsShape{
		"Shape",
		mParamSize,
		mParamResolution,
		mParamIsWireframe,
		mParamShouldDrawInfo,
	};

	ofParameter<bool> 	mParamDemoMode{ "Demo Mode", false };

	std::vector<ofEventListener> mParamListeners;

	ofxPanel mPanel1;

	ofShader mShdRender;

	ofTexture mTexture;

	public:
		void setup();
		void update();
		void draw();

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
		
};
