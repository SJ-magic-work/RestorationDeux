/************************************************************
************************************************************/
#include "ofApp.h"

#include <time.h>

/* for dir search */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h> 
#include <dirent.h>
#include <string>

using namespace std;


/************************************************************
************************************************************/
using namespace ofxCv;
using namespace cv;


/************************************************************
************************************************************/

/******************************
******************************/
ofApp::ofApp(int _Cam_id, bool _b_flipCamera)
: Cam_id(_Cam_id)
, c_CurrentActivePixels(0)
, c_TotalActivePixels(0)
, c_TotalActivePixels_Last(0)
, png_id(0)
, ArtPaint_id(0)
, b_flipCamera(_b_flipCamera)
{
	/********************
	********************/
	srand((unsigned) time(NULL));
	
	/********************
	********************/
	font[FONT_S].load("font/RictyDiminished-Regular.ttf", 10, true, true, true);
	font[FONT_M].load("font/RictyDiminished-Regular.ttf", 12, true, true, true);
	font[FONT_L].load("font/RictyDiminished-Regular.ttf", 15, true, true, true);
	font[FONT_LL].load("font/RictyDiminished-Regular.ttf", 30, true, true, true);
}

/******************************
******************************/
ofApp::~ofApp()
{
}

/******************************
******************************/
void ofApp::setup(){

	/********************
	********************/
	ofSetWindowTitle("RESTORATION");
	
	ofSetWindowShape( WINDOW_WIDTH, WINDOW_HEIGHT );
	/*
	ofSetVerticalSync(false);
	ofSetFrameRate(0);
	/*/
	ofSetVerticalSync(true);
	ofSetFrameRate(30);
	//*/
	
	ofSetEscapeQuitsApp(false);
	
	/********************
	********************/
	setup_Gui();
	
	setup_Camera();
	
	/********************
	********************/
	shader_AddMask.load( "sj_shader/AddMask.vert", "sj_shader/AddMask.frag");
	shader_Mask.load( "sj_shader/mask.vert", "sj_shader/mask.frag");
	
	/********************
	********************/
	/* */
	img_Frame.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_COLOR);
	img_Frame_Gray.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_LastFrame_Gray.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_AbsDiff_BinGray.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_BinGray_Cleaned.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_GRAYSCALE);
	img_Bin_RGB.allocate(CAM_WIDTH, CAM_HEIGHT, OF_IMAGE_COLOR);
	
	/* */
    fbo_CamFrame.allocate(ART_PAINT_WIDTH, ART_PAINT_HEIGHT, GL_RGBA);
    fbo_Paint.allocate(ART_PAINT_WIDTH, ART_PAINT_HEIGHT, GL_RGBA);
    fbo_Paint_Back.allocate(ART_PAINT_WIDTH, ART_PAINT_HEIGHT, GL_RGBA);
	
    fbo_Mask_S.allocate(CAM_WIDTH, CAM_HEIGHT, GL_RGBA);
	Reset_FboMask();
	pix_Mask_S.allocate(CAM_WIDTH, CAM_HEIGHT, GL_RGBA);
	ActivePoints_Of_Mask.resize(CAM_WIDTH * CAM_HEIGHT);
	
    fbo_Mask_L.allocate(ART_PAINT_WIDTH, ART_PAINT_HEIGHT, GL_RGBA, 4);
	
    fbo_PreOut.allocate(ART_PAINT_WIDTH, ART_PAINT_HEIGHT, GL_RGBA);
    fbo_Out.allocate(ART_PAINT_WIDTH, ART_PAINT_HEIGHT, GL_RGBA);
	
	
	/********************
	********************/
	myGlitch.setup(&fbo_Out);
	Clear_AllGlitch();
	
	/********************
	********************/
	makeup_image_list("../../../data/image", ArtPaints);
	Order_ArtPaints.resize(ArtPaints.size());
	SJ_UTIL::FisherYates(Order_ArtPaints);
	Refresh_Fbo_ArtPaint();
	
	/********************
	********************/
	SJ_UTIL::setup_sound(sound_Amb, "sound/main/airplane.wav", true, 0.5);
	sound_Amb.play();
	
	/********************
	********************/
	SyphonServer.setName("RESTORATION_DEUX");
}

/******************************
******************************/
void ofApp::inc_ArtPaint_id()
{
	ArtPaint_id = getNextId_of_ArtPaint();
}

/******************************
******************************/
int ofApp::getNextId_of_ArtPaint()
{
	int ret = ArtPaint_id + 1;
	if(ArtPaints.size() <= ret) ret = 0;
	
	return ret;
}

/******************************
******************************/
void ofApp::Reset_FboMask()
{
	fbo_Mask_S.begin();
	ofSetColor(255, 255, 255, 255);
	ofDrawRectangle(0, 0, fbo_Mask_S.getWidth(), fbo_Mask_S.getHeight());
	
	fbo_Mask_S.end();
}

/******************************
******************************/
void ofApp::Refresh_Fbo_ArtPaint()
{
	/* */
	fbo_Paint.begin();
	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	
	ArtPaints[Order_ArtPaints[ArtPaint_id]].draw(0, 0, fbo_Paint.getWidth(), fbo_Paint.getHeight());
	fbo_Paint.end();
	
	/* */
	fbo_Paint_Back.begin();
	ofClear(0, 0, 0, 0);
	ofSetColor(255, 255, 255, 255);
	
	ArtPaints[Order_ArtPaints[getNextId_of_ArtPaint()]].draw(0, 0, fbo_Paint_Back.getWidth(), fbo_Paint_Back.getHeight());
	fbo_Paint_Back.end();
}

/******************************
******************************/
void ofApp::makeup_image_list(const string dirname, vector<ofImage>& images)
{
	/********************
	********************/
	DIR *pDir;
	struct dirent *pEnt;
	struct stat wStat;
	string wPathName;

	pDir = opendir( dirname.c_str() );
	if ( NULL == pDir ) { ERROR_MSG(); std::exit(1); }

	pEnt = readdir( pDir );
	while ( pEnt ) {
		// .と..は処理しない
		if ( strcmp( pEnt->d_name, "." ) && strcmp( pEnt->d_name, ".." ) ) {
		
			wPathName = dirname + "/" + pEnt->d_name;
			
			// ファイルの情報を取得
			if ( stat( wPathName.c_str(), &wStat ) ) {
				printf( "Failed to get stat %s \n", wPathName.c_str() );
				break;
			}
			
			if ( S_ISDIR( wStat.st_mode ) ) {
				// nothing.
			} else {
			
				vector<string> str = ofSplitString(pEnt->d_name, ".");
				if( (str[str.size()-1] == "png") || (str[str.size()-1] == "jpg") || (str[str.size()-1] == "jpeg") ){
					ofImage _image;
					_image.load(wPathName);
					images.push_back(_image);
				}
			}
		}
		
		pEnt = readdir( pDir ); // 次のファイルを検索する
	}

	closedir( pDir );
	
	/********************
	********************/
	if(images.size() < 2)	{ ERROR_MSG();std::exit(1);}
	else					{ printf("> %d images loaded\n", int(images.size())); fflush(stdout); }
}

/******************************
******************************/
void ofApp::setup_Camera()
{
	/********************
	********************/
	printf("> setup camera\n");
	fflush(stdout);
	
	ofSetLogLevel(OF_LOG_VERBOSE);
    cam.setVerbose(true);
	
	vector< ofVideoDevice > Devices = cam.listDevices();// 上 2行がないと、List表示されない.
	
	if(Cam_id == -1){
		std::exit(1);
	}else{
		if(Devices.size() <= Cam_id) { ERROR_MSG(); std::exit(1); }
		
		cam.setDeviceID(Cam_id);
		cam.initGrabber(CAM_WIDTH, CAM_HEIGHT);
		
		printf("> Cam size asked = (%d, %d), actual = (%d, %d)\n", int(CAM_WIDTH), int(CAM_HEIGHT), int(cam.getWidth()), int(cam.getHeight()));
		fflush(stdout);
	}
	
	return;
}

/******************************
description
	memoryを確保は、app start後にしないと、
	segmentation faultになってしまった。
******************************/
void ofApp::setup_Gui()
{
	/********************
	********************/
	Gui_Global = new GUI_GLOBAL;
	Gui_Global->setup("RESTORATION", "gui.xml", 1000, 10);
}

/******************************
******************************/
void ofApp::update(){
	/********************
	********************/
	int now = ofGetElapsedTimeMillis();
	
	/********************
	********************/
    cam.update();
    if(cam.isFrameNew())	{ update_img_OnCam(); }
	
	update_img();
	
	/********************
	********************/
	StateChart_Top(now);
	StateChart_Noise(now);
	StateChart_Repair(now);
	
	StateRepair.update(now);
	if(StateRepair.draw(now, fbo_Mask_S)){
		ofVec2f NextTarget = SelectNextTargetToRepair();
		StateRepair.SetNextTarget(NextTarget);
	}
	
	/********************
	********************/
	printf("%5d, %5d, %5d, %5d, %5.0f\r", StateTop.State, StateNoise.State, StateRepair.State, StateRepair.b_DispCursor, ofGetFrameRate());
	fflush(stdout);
}

/******************************
******************************/
void ofApp::update_img_OnCam(){
	ofxCv::copy(cam, img_Frame);
	if(b_flipCamera) img_Frame.mirror(false, true);
	img_Frame.update();
	
	Copy_CamFrame_to_fbo();
	
	img_LastFrame_Gray = img_Frame_Gray;
	// img_LastFrame_Gray.update(); // drawしないので不要.
	
	convertColor(img_Frame, img_Frame_Gray, CV_RGB2GRAY);
	ofxCv::blur(img_Frame_Gray, ForceOdd((int)Gui_Global->BlurRadius));
	img_Frame_Gray.update();
}

/******************************
******************************/
void ofApp::Copy_CamFrame_to_fbo(){
	ofDisableAlphaBlending();
	
	fbo_CamFrame.begin();
		ofClear(0, 0, 0, 0);
		ofSetColor(255);
		
		img_Frame.draw(0, 0, fbo_CamFrame.getWidth(), fbo_CamFrame.getHeight());
	fbo_CamFrame.end();
}

/******************************
******************************/
void ofApp::update_img(){
	absdiff(img_Frame_Gray, img_LastFrame_Gray, img_AbsDiff_BinGray);
	threshold(img_AbsDiff_BinGray, Gui_Global->thresh_Diff_to_Bin);
	img_AbsDiff_BinGray.update();
	
	ofxCv::copy(img_AbsDiff_BinGray, img_BinGray_Cleaned);
	ofxCv::medianBlur(img_BinGray_Cleaned, ForceOdd((int)Gui_Global->MedianRadius));
	threshold(img_BinGray_Cleaned, Gui_Global->thresh_Medianed_to_Bin);
	img_BinGray_Cleaned.update();
	
	// ofxCv::copy(img_BinGray_Cleaned, img_Bin_RGB);
	convertColor(img_BinGray_Cleaned, img_Bin_RGB, CV_GRAY2RGB);
	img_Bin_RGB.update();
	
	ofPixels pix = img_Bin_RGB.getPixels();
	c_CurrentActivePixels = Count_ActivePixel(pix, ofColor(255, 255, 255));
	
	if((StateTop.State == STATE_TOP::STATE__RUN) && (StateRepair.State == STATE_REPAIR::STATE__STABLE)) Add_DiffArea_To_MaskArea();
	fbo_Mask_S.readToPixels(pix_Mask_S);
	c_TotalActivePixels_Last = c_TotalActivePixels;
	c_TotalActivePixels = Count_ActivePixel(pix_Mask_S, ofColor(0, 0, 0));
	
	Fbo_MaskS_to_MaskL();
	
	Mask_x_ArtPaint();
	
	drawFbo_Preout_to_Out();
}

/******************************
******************************/
void ofApp::StateChart_Top(int now){
	switch(StateTop.State){
		case STATE_TOP::STATE__SLEEP:
			break;
			
		case STATE_TOP::STATE__RUN:
			if( StateTop.IsTimeout(now) ){
				Reset_FboMask();
				Random_Enable_myGlitch();
				StateTop.Transition(STATE_TOP::STATE__CHANGING_CONTENTS, now);
				
				StateNoise.Transition(STATE_NOISE::STATE__CALM, now);
				StateRepair.Transition(STATE_REPAIR::STATE__STABLE, now);
			}
			break;
			
		case STATE_TOP::STATE__CHANGING_CONTENTS:
			if( StateTop.IsTimeout(now) ){
				Clear_AllGlitch();
				
				inc_ArtPaint_id();
				Refresh_Fbo_ArtPaint();
				
				StateTop.Transition(STATE_TOP::STATE__WAIT_STABLE, now);
			}
			
			break;
			
		case STATE_TOP::STATE__WAIT_STABLE:
			if( StateTop.IsTimeout(now) ){
				StateTop.Transition(STATE_TOP::STATE__RUN, now);
			}
			break;
	}
}


/******************************
******************************/
void ofApp::StateChart_Noise(int now){

	if( !((StateTop.State == STATE_TOP::STATE__RUN) && (StateRepair.State == STATE_REPAIR::STATE__STABLE)) ) return;
	
	switch(StateNoise.State){
		case STATE_NOISE::STATE__CALM:
			// if(0 < c_CurrentActivePixels){
			if(c_TotalActivePixels_Last < c_TotalActivePixels){
				StateNoise.Transition(STATE_NOISE::STATE__NOISE, now);
				// Random_Enable_myGlitch();
			}
			break;
			
		case STATE_NOISE::STATE__NOISE:
			// if(c_CurrentActivePixels <= 0){
			if(c_TotalActivePixels <= c_TotalActivePixels_Last){
				Clear_AllGlitch();
				StateNoise.Transition(STATE_NOISE::STATE__ECHO, now);
			}
			break;
			
		case STATE_NOISE::STATE__ECHO:
			if(StateNoise.IsTimeout(now)){
				StateNoise.Transition(STATE_NOISE::STATE__CALM, now);
			// }else if(0 < c_CurrentActivePixels){
			}else if(c_TotalActivePixels_Last < c_TotalActivePixels){
				StateNoise.Transition(STATE_NOISE::STATE__NOISE, now);
				// Random_Enable_myGlitch();
			}
			break;
	}
}

/******************************
******************************/
void ofApp::StateChart_Repair(int now){
	if(StateTop.State != STATE_TOP::STATE__RUN)	return;
	
	switch(StateRepair.State){
		case STATE_REPAIR::STATE__STABLE:
			if( (StateNoise.State == STATE_NOISE::STATE__CALM) && (fbo_Mask_S.getWidth() * fbo_Mask_S.getHeight() * 0.7 < c_TotalActivePixels) ){
				StateRepair.Transition(STATE_REPAIR::STATE__WAIT_POWER_REPAIR, now);
			
			}else if( (StateNoise.State == STATE_NOISE::STATE__CALM) && (0 < c_TotalActivePixels) ){
				ofVec2f NextTarget = SelectNextTargetToRepair();
				
				StateRepair.SetNextTarget(NextTarget);
				StateRepair.Transition(STATE_REPAIR::STATE__REPAIR, now);
				
				StateRepair.Set_PatchSize(c_TotalActivePixels);
			}
			break;
			
		case STATE_REPAIR::STATE__REPAIR:
			break;
			
		case STATE_REPAIR::STATE__RAPID:
			if(c_TotalActivePixels <= 0){
				StateRepair.Transition(STATE_REPAIR::STATE__BUFFER, now);
				Reset_FboMask();
			}else if(3 < StateRepair.c_Rapid){
				StateRepair.Transition(STATE_REPAIR::STATE__BREAK, now);
			}
			
			break;
			
		case STATE_REPAIR::STATE__BREAK:
			if(StateRepair.IsTimeout(now)){
				if(0 < c_TotalActivePixels){
					ofVec2f NextTarget = SelectNextTargetToRepair();
					
					StateRepair.SetNextTarget(NextTarget);
					
					/* */
					int RandomNum = (int)( ((double)rand() / ((double)RAND_MAX + 1)) * 11 );

					if(RandomNum < 9)	StateRepair.Transition(STATE_REPAIR::STATE__REPAIR, now);
					else				StateRepair.Transition(STATE_REPAIR::STATE__RAPID, now);
					
				}else{
					StateRepair.Transition(STATE_REPAIR::STATE__BUFFER, now);
					Reset_FboMask();
				}
			}
			break;
			
		case STATE_REPAIR::STATE__BUFFER:
			if(StateRepair.IsTimeout(now)){
				StateRepair.Transition(STATE_REPAIR::STATE__STABLE, now);
			}
			break;
			
		case STATE_REPAIR::STATE__WAIT_POWER_REPAIR:
			if(StateRepair.IsTimeout(now)){
				StateRepair.Transition(STATE_REPAIR::STATE__POWER_REPAIR, now);
				Random_Enable_myGlitch();
			}
			break;
			
		case STATE_REPAIR::STATE__POWER_REPAIR:
			if(StateRepair.IsTimeout(now)){
				StateRepair.Transition(STATE_REPAIR::STATE__BUFFER, now);
				Clear_AllGlitch();
				Reset_FboMask();
			}
			break;
	}
}

/******************************
******************************/
ofVec2f ofApp::SizeConvert_StoL(ofVec2f from){
	return from * (float(ART_PAINT_WIDTH) / CAM_WIDTH);
}

/******************************
******************************/
ofVec2f ofApp::SelectNextTargetToRepair(){
	fbo_Mask_S.readToPixels(pix_Mask_S);
	
	int counter = 0;
	
	for(int _y = 0; _y < pix_Mask_S.getHeight(); _y++){
		for(int _x = 0; _x < pix_Mask_S.getWidth(); _x++){
			ofColor col = pix_Mask_S.getColor(_x, _y);
			if( (col.r <= 0) && (col.g <= 0) && (col.b <= 0) ){
				ActivePoints_Of_Mask[counter].x = _x;
				ActivePoints_Of_Mask[counter].y = _y;
				
				counter++;
			}
		}
	}
	
	int id = ofRandom(0, counter);
	return ActivePoints_Of_Mask[id];
}

/******************************
******************************/
void ofApp::drawFbo_Preout_to_Out()
{
	/********************
	********************/
	ofDisableAlphaBlending();
	
	fbo_Out.begin();
	ofClear(0, 0, 0, 0);

	ofSetColor(255, 255, 255, 255);
	
	fbo_PreOut.draw(0, 0, fbo_Out.getWidth(), fbo_Out.getHeight());
	
	if(StateRepair.b_DispCursor){
		ofVec2f CursorPos = StateRepair.get_CursorPos();
		CursorPos = SizeConvert_StoL(CursorPos);
		
		/********************
		oFでは、加算合成とスムージングは同時に効かない。その為、加算合成の前にofDisableSmoothing()を記述。
			https://qiita.com/y_UM4/items/b03a66d932536b25b51a
		********************/
		ofEnableAlphaBlending();
		// ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		// ofDisableAlphaBlending();
		
		ofSetColor(255, 255, 0, 200);
		glPointSize(1.0);
		// glLineWidth(1);
		ofSetLineWidth(2);
		
		ofDrawLine(0, CursorPos.y, fbo_Out.getWidth(), CursorPos.y);
		ofDrawLine(CursorPos.x, 0, CursorPos.x, fbo_Out.getHeight());
	}
	fbo_Out.end();
	
	/********************
	********************/
	if( (StateRepair.State != STATE_REPAIR::STATE__STABLE) && (StateRepair.State != STATE_REPAIR::STATE__BUFFER) ){
		drawFbo_Scale(fbo_Out);
		drawFbo_String_Info_on_Repairing(fbo_Out);
		// drawFbo_String_TotalActivePixels(fbo_Out);
	}
	
	if(StateRepair.State == STATE_REPAIR::STATE__POWER_REPAIR) drawFbo_String_PowerRepairing(fbo_Out);
}

/******************************
******************************/
void ofApp::drawFbo_String_Info_on_Repairing(ofFbo& fbo)
{
	/********************
	********************/
	ofEnableAlphaBlending();
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	
	/********************
	********************/
	char buf[BUF_SIZE_S];
	const int X0		= 900;
	const int X1		= 1090;
	const int Y_INIT	= 70;
	const int Y_STEP	= 50;
	
	fbo.begin();
		/********************
		********************/
		ofSetColor(0, 0, 0, 100);
		ofDrawRectangle(fbo.getWidth() * 2/3, 0, fbo.getWidth() /3, fbo.getHeight());
		
		/********************
		********************/
		
		ofSetColor(255, 255, 255, 200);
		
		/* */
		int _y = Y_INIT;
		{
			sprintf(buf, "to be Repaired");
			font[FONT_L].drawString(buf, X0, _y);
			
			sprintf(buf, "%6d [pix]", c_TotalActivePixels);
			font[FONT_L].drawString(buf, X1, _y);
		}
		
		/* */
		{
			_y += Y_STEP;
			
			sprintf(buf, "to be Repaired");
			font[FONT_L].drawString(buf, X0, _y);
			
			sprintf(buf, "%6.2f [%%]", 100 * float(c_TotalActivePixels) / float(CAM_WIDTH * CAM_HEIGHT));
			font[FONT_L].drawString(buf, X1, _y);
		}
		
		/* */
		{
			_y += Y_STEP;
			
			ofVec2f CursorPos = StateRepair.get_CursorPos();
			CursorPos = SizeConvert_StoL(CursorPos);
			
			sprintf(buf, "Cursor Current");
			font[FONT_L].drawString(buf, X0, _y);
			
			
			sprintf(buf, "%6d, %6d", (int)CursorPos.x, (int)CursorPos.y);
			font[FONT_L].drawString(buf, X1, _y);
		}
		
		/* */
		{
			_y += Y_STEP;
			
			ofVec2f CursorPos = StateRepair.get_TargetCursorPos();
			CursorPos = SizeConvert_StoL(CursorPos);
			
			sprintf(buf, "Cursor Target");
			font[FONT_L].drawString(buf, X0, _y);
			
			
			sprintf(buf, "%6d, %6d", (int)CursorPos.x, (int)CursorPos.y);
			font[FONT_L].drawString(buf, X1, _y);
		}
		
		/* */
		{
			_y += Y_STEP;
			
			ofVec2f CursorPos = StateRepair.get_TargetCursorPos();
			CursorPos = SizeConvert_StoL(CursorPos);
			
			sprintf(buf, "Patch Size");
			font[FONT_L].drawString(buf, X0, _y);
			
			
			sprintf(buf, "%6d", (int)StateRepair.PatchSize);
			font[FONT_L].drawString(buf, X1, _y);
		}
		
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_String_PowerRepairing(ofFbo& fbo)
{
	ofDisableAlphaBlending();
	
	fbo.begin();
	ofSetColor(255, 0, 0, 255);
	
	char buf[BUF_SIZE_S];
	sprintf(buf, "REPAIRING NOW...");
	font[FONT_LL].drawString(buf, fbo.getWidth()/2 - font[FONT_LL].stringWidth(buf)/2, fbo.getHeight()/2);
	
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_String_TotalActivePixels(ofFbo& fbo)
{
	/********************
	********************/
	ofDisableAlphaBlending();
	
	fbo.begin();
	
	ofSetColor(255, 255, 0, 255);
	
	char buf[BUF_SIZE_S];
	sprintf(buf, "Noise:%6d", c_TotalActivePixels);
	font[FONT_L].drawString(buf, fbo.getWidth() - font[FONT_L].stringWidth("Noise:XXXXXX") - 50, fbo.getHeight()/2 /*font[FONT_L].stringHeight(buf) * 2*/);
	
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_Scale(ofFbo& fbo)
{
	/* */
	drawFbo_Scale_x(fbo, ofVec2f(0, 0), ofVec2f(1, 1));
	drawFbo_Scale_x(fbo, ofVec2f(0, fbo.getHeight()), ofVec2f(1, -1));
	
	drawFbo_ScaleLine_x(fbo);
	
	/* */
	drawFbo_Scale_y(fbo, ofVec2f(0, 0), ofVec2f(1, 1));
	drawFbo_Scale_y(fbo, ofVec2f(fbo.getWidth(), 0), ofVec2f(-1, 1));
	
	drawFbo_ScaleLine_y(fbo);
}

/******************************
******************************/
void ofApp::drawFbo_ScaleLine_x(ofFbo& fbo)
{
	ofDisableAlphaBlending();
	
	fbo.begin();
		
		ofSetColor(255, 255, 255, 80);
		glPointSize(1.0);
		// glLineWidth(1);
		ofSetLineWidth(1);
		
		/********************
		********************/
		const float step = 5;
		const float space = 20;
		
		int id;
		float _x;
		for(id = 0, _x = 0; _x < fbo.getWidth(); _x += step, id++){
			if(id % 10 == 0)	ofDrawLine(_x, space, _x, fbo.getHeight() - space);
		}
		
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_ScaleLine_y(ofFbo& fbo)
{
	ofDisableAlphaBlending();
	
	fbo.begin();
		
		ofSetColor(255, 255, 255, 80);
		glPointSize(1.0);
		// glLineWidth(1);
		ofSetLineWidth(1);
		
		/********************
		********************/
		const float step = 5;
		const float space = 20;
		
		int id;
		float _y;
		for(id = 0, _y = 0; _y < fbo.getHeight(); _y += step, id++){
			if(id % 10 == 0)		ofDrawLine(space, _y, fbo.getWidth() - space, _y);
		}
		
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_Scale_x(ofFbo& fbo, ofVec2f _pos, ofVec2f _scale)
{
	ofDisableAlphaBlending();
	
	fbo.begin();
		
		ofSetColor(255, 255, 255, 180);
		glPointSize(1.0);
		// glLineWidth(1);
		ofSetLineWidth(1);
		
		/********************
		********************/
		const float step = 5;
		
		ofPushMatrix();
			ofTranslate(_pos);
			ofScale(_scale.x, _scale.y);
			
			int id;
			float _x;
			for(id = 0, _x = 0; _x < fbo.getWidth(); _x += step, id++){
				if(id % 10 == 0)		ofDrawLine(_x, 0, _x, 15);
				else if(id % 5 == 0)	ofDrawLine(_x, 0, _x, 10);
				else					ofDrawLine(_x, 0, _x, 5);
			}
			
		ofPopMatrix();
		
	fbo.end();
}

/******************************
******************************/
void ofApp::drawFbo_Scale_y(ofFbo& fbo, ofVec2f _pos, ofVec2f _scale)
{
	ofDisableAlphaBlending();
	
	fbo.begin();
		
		ofSetColor(255, 255, 255, 180);
		glPointSize(1.0);
		// glLineWidth(1);
		ofSetLineWidth(1);
		
		/********************
		********************/
		const float step = 5;
		
		ofPushMatrix();
			ofTranslate(_pos);
			ofScale(_scale.x, _scale.y);
			
			int id;
			float _y;
			for(id = 0, _y = 0; _y < fbo.getHeight(); _y += step, id++){
				if(id % 10 == 0)		ofDrawLine(0, _y, 15, _y);
				else if(id % 5 == 0)	ofDrawLine(0, _y, 10, _y);
				else					ofDrawLine(0, _y, 5, _y);
			}
			
		ofPopMatrix();
		
	fbo.end();
}

/******************************
******************************/
void ofApp::Add_DiffArea_To_MaskArea()
{
	ofDisableAlphaBlending();
	
	fbo_Mask_S.begin();
	shader_AddMask.begin();
	
		// ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		shader_AddMask.setUniformTexture( "MaskAll", fbo_Mask_S.getTexture(), 1 );
		
		img_Bin_RGB.draw(0, 0, fbo_Mask_S.getWidth(), fbo_Mask_S.getHeight());
		
	shader_AddMask.end();
	fbo_Mask_S.end();
}

/******************************
******************************/
void ofApp::Fbo_MaskS_to_MaskL()
{
	ofDisableAlphaBlending();
	ofEnableSmoothing();
	
	fbo_Mask_L.begin();
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		fbo_Mask_S.draw(0, 0, fbo_Mask_L.getWidth(), fbo_Mask_L.getHeight());
		
	fbo_Mask_L.end();
}

/******************************
******************************/
void ofApp::Mask_x_ArtPaint()
{
	ofDisableAlphaBlending();
	
	fbo_PreOut.begin();
	shader_Mask.begin();
	
		ofClear(0, 0, 0, 0);
		ofSetColor(255, 255, 255, 255);
		
		// shader_Mask.setUniformTexture( "Back", fbo_Paint_Back.getTexture(), 1 );
		shader_Mask.setUniformTexture( "Back", fbo_CamFrame.getTexture(), 1 );
		shader_Mask.setUniformTexture( "mask", fbo_Mask_L.getTexture(), 2 );
		
		fbo_Paint.draw(0, 0, fbo_PreOut.getWidth(), fbo_PreOut.getHeight());
		
	shader_Mask.end();
	fbo_PreOut.end();
}

/******************************
******************************/
int ofApp::Count_ActivePixel(ofPixels& pix, ofColor val)
{
	int counter = 0;
	
	for(int _y = 0; _y < pix.getHeight(); _y++){
		for(int _x = 0; _x < pix.getWidth(); _x++){
			ofColor col = pix.getColor(_x, _y);
			if( (col.r == val.r) && (col.g == val.g) && (col.b == val.b) ) counter++;
		}
	}
	
	return counter;
}

/******************************
******************************/
int ofApp::ForceOdd(int val){
	if(val <= 0)			return val;
	else if(val % 2 == 0)	return val - 1;
	else					return val;
}

/******************************
******************************/
void ofApp::draw(){
	/********************
	********************/
	/*
	ofEnableAlphaBlending();
	// ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	*/
	ofDisableAlphaBlending();
	
	/********************
	********************/
	// ofClear(0, 0, 0, 0);
	ofBackground(50);
	ofSetColor(255, 255, 255, 255);
	
	/********************
	********************/
	myGlitch.generateFx(); // Apply effects
	
	/********************
	********************/
	draw_ProcessLine();
	
	draw_ProcessedImages();
	
	/********************
	********************/
	ofTexture tex = fbo_Out.getTextureReference();
	SyphonServer.publishTexture(&tex);
	
	/********************
	********************/
	Gui_Global->gui.draw();
}

/******************************
******************************/
void ofApp::draw_ProcessLine(){
	/********************
	********************/
	ofSetColor(250);
	glPointSize(1.0);
	// glLineWidth(1);
	ofSetLineWidth(1);
	
	
	ofDrawLine(240, 135, 1040, 135);
	ofDrawLine(240, 360, 1040, 360);
	ofDrawLine(240, 585, 1040, 585);
	
	ofDrawLine(1040, 135, 1040, 360);
	ofDrawLine(240, 360, 240, 585);
}

/******************************
******************************/
void ofApp::draw_ProcessedImages(){
	/********************
	********************/
	ofSetColor(255, 255, 255, 255);
	
	/********************
	********************/
	/* */
	img_Frame.draw(80, 45, DRAW_WIDTH, DRAW_HEIGHT);
	img_Frame_Gray.draw(480, 45, DRAW_WIDTH, DRAW_HEIGHT);
	img_AbsDiff_BinGray.draw(880, 45, DRAW_WIDTH, DRAW_HEIGHT);
	
	/* */
	img_BinGray_Cleaned.draw(880, 270, DRAW_WIDTH, DRAW_HEIGHT);
	
	char buf[BUF_SIZE_S];
	sprintf(buf, "%6d / %6d", c_CurrentActivePixels, int(img_Bin_RGB.getWidth() * img_Bin_RGB.getHeight()));
	font[FONT_M].drawString(buf, 1200 - font[FONT_M].stringWidth(buf), 450 + font[FONT_M].stringHeight(buf) * 1.1);
	
	/* */
	fbo_Mask_S.draw(480, 270, DRAW_WIDTH, DRAW_HEIGHT);
	
	sprintf(buf, "%6d / %6d", c_TotalActivePixels, int(fbo_Mask_S.getWidth() * fbo_Mask_S.getHeight()));
	font[FONT_M].drawString(buf, 800 - font[FONT_M].stringWidth(buf), 450 + font[FONT_M].stringHeight(buf) * 1.1);
	
	/* */
	fbo_Paint_Back.draw(80, 270, DRAW_WIDTH, DRAW_HEIGHT);
	fbo_Paint.draw(80, 495, DRAW_WIDTH, DRAW_HEIGHT);
	fbo_PreOut.draw(480, 495, DRAW_WIDTH, DRAW_HEIGHT);
	
	fbo_Out.draw(880, 495, DRAW_WIDTH, DRAW_HEIGHT);
	if(StateRepair.State != STATE_REPAIR::STATE__STABLE){
		sprintf(buf, "%6d", int(StateRepair.PatchSize));
		font[FONT_M].drawString(buf, 1200 - font[FONT_M].stringWidth(buf), 675 + font[FONT_M].stringHeight(buf) * 1.1);
	}
}

/******************************
******************************/
void ofApp::Random_Enable_myGlitch(){
	SJ_UTIL::FisherYates(Glitch_Ids, NUM_GLITCH_TYPES);
	NumGlitch_Enable = int(ofRandom(1, MAX_NUM_GLITCHS_ONE_TIME));
	for(int i = 0; i < NumGlitch_Enable; i++) { Set_myGlitch(Glitch_Ids[i], true); }
}

/******************************
******************************/
void ofApp::Set_myGlitch(int key, bool b_switch)
{
	if (key == 0)	myGlitch.setFx(OFXPOSTGLITCH_INVERT			, b_switch);
	if (key == 1)	myGlitch.setFx(OFXPOSTGLITCH_CONVERGENCE	, b_switch);
	if (key == 2)	myGlitch.setFx(OFXPOSTGLITCH_GLOW			, b_switch);
	if (key == 3)	myGlitch.setFx(OFXPOSTGLITCH_SHAKER			, b_switch);
	if (key == 4)	myGlitch.setFx(OFXPOSTGLITCH_CUTSLIDER		, b_switch);
	if (key == 5)	myGlitch.setFx(OFXPOSTGLITCH_TWIST			, b_switch);
	if (key == 6)	myGlitch.setFx(OFXPOSTGLITCH_OUTLINE		, b_switch);
	if (key == 7)	myGlitch.setFx(OFXPOSTGLITCH_NOISE			, b_switch);
	if (key == 8)	myGlitch.setFx(OFXPOSTGLITCH_SLITSCAN		, b_switch);
	if (key == 9)	myGlitch.setFx(OFXPOSTGLITCH_SWELL			, b_switch);
	if (key == 10)	myGlitch.setFx(OFXPOSTGLITCH_CR_HIGHCONTRAST, b_switch);
	if (key == 11)	myGlitch.setFx(OFXPOSTGLITCH_CR_BLUERAISE	, b_switch);
	if (key == 12)	myGlitch.setFx(OFXPOSTGLITCH_CR_REDRAISE	, b_switch);
	if (key == 13)	myGlitch.setFx(OFXPOSTGLITCH_CR_GREENRAISE	, b_switch);
	if (key == 14)	myGlitch.setFx(OFXPOSTGLITCH_CR_BLUEINVERT	, b_switch);
	if (key == 15)	myGlitch.setFx(OFXPOSTGLITCH_CR_REDINVERT	, b_switch);
	if (key == 16)	myGlitch.setFx(OFXPOSTGLITCH_CR_GREENINVERT	, b_switch);
}

/******************************
******************************/
void ofApp::Clear_AllGlitch()
{
	for(int i = 0; i < NUM_GLITCH_TYPES; i++){
		Set_myGlitch(i, false);
	}
}

/******************************
******************************/
void ofApp::keyPressed(int key){
	switch(key){
		case 'r':
			if(StateTop.State == STATE_TOP::STATE__SLEEP){
				StateTop.Transition(STATE_TOP::STATE__RUN, ofGetElapsedTimeMillis());
				Clear_AllGlitch();
			}
			break;
			
		case 's':
			if(StateTop.State == STATE_TOP::STATE__RUN){
				int now = ofGetElapsedTimeMillis();
				
				StateTop.Transition(STATE_TOP::STATE__SLEEP, now);
				Reset_FboMask();
				StateNoise.Transition(STATE_NOISE::STATE__CALM, now);
				StateRepair.Transition(STATE_REPAIR::STATE__STABLE, now);
				Clear_AllGlitch();
			}
			break;
			
			
		case ' ':
			{
				char buf[512];
				
				sprintf(buf, "image_%d.png", png_id);
				ofSaveScreen(buf);
				// ofSaveFrame();
				printf("> %s saved\n", buf);
				
				png_id++;
			}
			
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
