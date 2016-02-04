#pragma once

#include "ofMain.h"
#include "ofxMidiFighter.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

	pal::Kontrol::MidiFighter mKontrol1;

	ofParameterGroup mParamGroupA{ "group A",
		ofParameter<float>{"float a 0", 0.5, 0.0, 1.0}, 
		ofParameter<float>{"float a 1", 0.5, 0.0, 1.0},
		ofParameter<float>{"float a 2", 0.5, 0.0, 1.0},
	};
	
	ofParameterGroup mParamGroupB{ "group B",
		ofParameter<float>{"float b 0", 0.5, 0.0, 1.0},
		ofParameter<bool>{"bool b 1", true},
		ofParameter<float>{"float b 2", 0.5, 0.0, 1.0},
	};

	ofParameterGroup mParamAlias = mParamGroupA;

	ofxPanel mPanel1;

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
