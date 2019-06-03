#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( int argc, char** argv ){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	/********************
	********************/
	int Cam_id = -1;
	bool b_flipCamera = false;

	/********************
	********************/
	printf("---------------------------------\n");
	printf("> parameters\n");
	printf("\t-c:camera(-1)\n");
	printf("\t-f:flip camera(false)\n");
	printf("---------------------------------\n");
	
	for(int i = 1; i < argc; i++){
		if( strcmp(argv[i], "-c") == 0 ){
			if(i+1 < argc) Cam_id = atoi(argv[i+1]);
		}else if( strcmp(argv[i], "-f") == 0 ){
			if(i+1 < argc) b_flipCamera = atoi(argv[i+1]);
		}
	}
	
	ofRunApp(new ofApp(Cam_id, b_flipCamera));
}
