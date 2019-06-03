/************************************************************
■ofxCv
	https://github.com/kylemcdonald/ofxCv
	
	note
		Your linker will also need to know where the OpenCv headers are. In XCode this means modifying one line in Project.xconfig:
			HEADER_SEARCH_PATHS = $(OF_CORE_HEADERS) "../../../addons/ofxOpenCv/libs/opencv/include/" "../../../addons/ofxCv/libs/ofxCv/include/"
			
		
		
■openFrameworksのAddon、ofxCvの導入メモ
	https://qiita.com/nenjiru/items/50325fabe4c3032da270
	
	contents
		導入時に、一手間 必要.
		
■Kill camera process
	> sudo killall VDCAssistant
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"
#include "ofxCv.h"
#include "ofxPostGlitch.h"
#include "ofxSyphon.h"

#include "sj_common.h"
#include "util.h"

/************************************************************
************************************************************/

/**************************************************
**************************************************/
struct STATE_TOP{
	enum STATE{
		STATE__SLEEP,
		STATE__RUN,
		STATE__CHANGING_CONTENTS,
		STATE__WAIT_STABLE,
		
		NUM_STATE,
	};
	
	ofSoundPlayer sound_Noise;
	float MaxVol_SoundNoise;
	
	STATE State;
	int t_from_ms;
	int duration;
	
	STATE_TOP()
	: State(STATE__SLEEP)
	, duration(2000)
	, MaxVol_SoundNoise(1.0)
	{
		SJ_UTIL::setup_sound(sound_Noise, "sound/top/radio-tuning-noise-short-waves_zydE-HEd.mp3", true, 0.0);
		sound_Noise.play();
	}
	
	void Transition(int NextState, int now)
	{
		if(NextState < NUM_STATE){
			State = STATE(NextState);
			t_from_ms = now;
			
			switch(State){
				case STATE__SLEEP:
					sound_Noise.setVolume(0);
					break;
					
				case STATE__RUN:
					duration = (int)ofRandom(60e+3, 70e+3);
					break;
					
				case STATE__CHANGING_CONTENTS:
					duration = (int)ofRandom(500, 600);
					sound_Noise.setVolume(MaxVol_SoundNoise);
					break;
					
				case STATE__WAIT_STABLE:
					duration = 1e+3;
					sound_Noise.setVolume(0);
					break;
			}
		}
	}
	
	int ms_To_Timeout(int now){
		return duration - (now - t_from_ms);
	}
	
	bool IsTimeout(int now){
		if(duration < now - t_from_ms)	return true;
		else							return false;
	}
};
	
/**************************************************
**************************************************/
struct STATE_NOISE{
	enum STATE{
		STATE__CALM,
		STATE__NOISE,
		STATE__ECHO,
		
		NUM_STATE,
	};
	
	ofSoundPlayer sound_Noise;
	float MaxVol_SoundNoise;
	
	STATE State;
	int t_from_ms;
	const int duration;
	
	STATE_NOISE()
	: State(STATE__CALM)
	, duration(1000)
	, MaxVol_SoundNoise(1.0)
	{
		SJ_UTIL::setup_sound(sound_Noise, "sound/Noise/test-tone-pink-noise_Gy3tO2Vd.wav", true, 0.0);
		sound_Noise.play();
	}

	void Transition(int NextState, int now)
	{
		if(NextState < NUM_STATE){
			State = STATE(NextState);
			t_from_ms = now;
		}
		
		switch(State){
			case STATE__CALM:
				sound_Noise.setVolume(0);
				break;
				
			case STATE__NOISE:
				sound_Noise.setVolume(MaxVol_SoundNoise);
				break;
				
			case STATE__ECHO:
				sound_Noise.setVolume(0);
				break;
		}
	}
	
	bool IsTimeout(int now){
		if(duration < now - t_from_ms)	return true;
		else							return false;
	}
};
	
/**************************************************
**************************************************/
struct STATE_REPAIR{
private:
	void SoundHeart__Vol_UpDown(int now){
		float Speed = 0.5;
		
		float _vol = sound_Heart.getVolume();
		if(b_DispCursor){
			_vol += Speed * (now - LastInt) * 1e-3;
			if(1.0 < _vol) _vol = 1.0;
		}else{
			_vol -= Speed * (now - LastInt) * 1e-3;
			if(_vol < 0.0) _vol = 0.0;
		}
		
		sound_Heart.setVolume(_vol);
	}
	
public:
	enum STATE{
		STATE__STABLE,
		STATE__REPAIR,
		STATE__RAPID,
		STATE__BREAK,
		STATE__BUFFER,
		STATE__WAIT_POWER_REPAIR,
		STATE__POWER_REPAIR,
		
		NUM_STATE,
	};
	
	ofSoundPlayer sound_Pi;
	ofSoundPlayer sound_Rapid;
	ofSoundPlayer sound_Heart;
	ofSoundPlayer sound_PowerRepair;
	float Vol_Sound_Pi;
	float MaxVol_Sound_Heart;
	float MaxVol_Sound_PowerRepair;
	int t_LastSound_Pi;
	
	STATE State;
	int t_from_ms;
	int duration;
	int LastInt;
	
	bool b_DispCursor;
	int c_Rapid;
	
	ofVec2f Cross;
	ofVec2f Cross_from;
	ofVec2f Cross_Next;
	ofVec2f dir;
	float SquareDistance;
	const float RepairSpeed;
	
	float PatchSize;
	
	STATE_REPAIR()
	: State(STATE__STABLE)
	, Vol_Sound_Pi(0.7)
	, MaxVol_Sound_Heart(1.0)
	, b_DispCursor(false)
	, LastInt(0)
	, t_LastSound_Pi(0)
	, Cross(ofVec2f(0, 0))
	, RepairSpeed(140)
	// , PatchSize(50)
	, PatchSize(50)
	, MaxVol_Sound_PowerRepair(1.0)
	{
		SJ_UTIL::setup_sound(sound_Pi, "sound/Repair/pi_1.mp3", false, Vol_Sound_Pi);
		SJ_UTIL::setup_sound(sound_Rapid, "sound/Repair/Cash_Register-Beep01-1.mp3", false, 0.3);
		
		SJ_UTIL::setup_sound(sound_Heart, "sound/Repair/electrocardiogram_Loop.mp3", true, MaxVol_Sound_Heart);
		sound_Heart.play();
		
		SJ_UTIL::setup_sound(sound_PowerRepair, "sound/Repair/computer-keyboard-type-long_GJE_D24u.mp3", true, 0.0);
		sound_PowerRepair.play();
	}
	
	void Transition(int NextState, int now)
	{
		if(NextState < NUM_STATE){
			State = STATE(NextState);
			t_from_ms = now;
		}
		
		switch(State){
			case STATE__STABLE:
				b_DispCursor = false;
				sound_PowerRepair.setVolume(0.0);
				break;
				
			case STATE__REPAIR:
				b_DispCursor = true;
				if(10e+3 < now - t_LastSound_Pi) { sound_Pi.play(); t_LastSound_Pi = now; }
				break;
				
			case STATE__RAPID:
				duration = 80;
				c_Rapid = 0;
				break;
				
			case STATE__BREAK:
				duration = 50;
				break;
				
			case STATE__BUFFER:
				duration = 1000;
				b_DispCursor = false;
				sound_PowerRepair.setVolume(0.0);
				break;
				
			case STATE__WAIT_POWER_REPAIR:
				duration = 1500;
				break;
				
			case STATE__POWER_REPAIR:
				duration = (int)ofRandom(2500, 3000);
				sound_PowerRepair.setVolume(MaxVol_Sound_PowerRepair);
				break;
		}
	}
	
	void Set_PatchSize(float Rest){
		float _size = Rest / 5;
		_size = sqrt(_size);
		
		if(_size < 50) _size = 50;
		PatchSize = _size;
	}
	
	bool IsTimeout(int now){
		if(duration < now - t_from_ms)	return true;
		else							return false;
	}
	
	void SetNextTarget(ofVec2f _NextTarget){
		Cross_Next = _NextTarget;
		
		SquareDistance = Cross.squareDistance(Cross_Next);
		
		dir = Cross_Next - Cross;
		dir.normalize();
		
		Cross_from = Cross;
	}
	
	void update(int now){
		// SoundHeart__Vol_UpDown(now);
		
		if(State == STATE__REPAIR){
			ofVec2f NextPos = Cross + dir * (RepairSpeed * (now - LastInt) * 1e-3);
			if(SquareDistance <= Cross_from.squareDistance(NextPos))	Cross = Cross_Next;
			else														Cross = NextPos;
		}
		
		LastInt = now;
	}
	
	/******************************
	when ret == true : Please Change Next Target.
	******************************/
	bool draw(int now, ofFbo& fbo){
		bool ret = false;
		
		if(State == STATE__REPAIR){
			if(Cross == Cross_Next){
				State = STATE__BREAK;
				
				fbo.begin();
				ofSetColor(255, 255, 255, 255);
				// ofDrawRectRounded(Cross.x - PatchSize/2, Cross.y - PatchSize/2, PatchSize, PatchSize, 10);
				ofDrawRectangle(Cross.x - PatchSize/2, Cross.y - PatchSize/2, PatchSize, PatchSize);
				fbo.end();
				
				sound_Pi.play();
				t_LastSound_Pi = now;
				
				Transition(STATE__BREAK, now);
			}
			
		}else if(State == STATE__RAPID){
			if(duration < now - t_from_ms){
				t_from_ms = now;
				c_Rapid++;
				Cross = Cross_Next;
				
				fbo.begin();
				ofSetColor(255, 255, 255, 255);
				// ofDrawRectRounded(Cross.x - PatchSize/2, Cross.y - PatchSize/2, PatchSize, PatchSize, 10);
				ofDrawRectangle(Cross.x - PatchSize/2, Cross.y - PatchSize/2, PatchSize, PatchSize);
				fbo.end();
				
				sound_Rapid.play();
				
				ret = true;
			}
		}
		
		return ret;
	}
	
	ofVec2f get_CursorPos(){
		return Cross;
	}
	
	ofVec2f get_TargetCursorPos(){
		return Cross_Next;
	}
};
	

/**************************************************
**************************************************/
class ofApp : public ofBaseApp{
private:
	/****************************************
	****************************************/
	enum{
		FONT_S,
		FONT_M,
		FONT_L,
		FONT_LL,
		
		NUM_FONTSIZE,
	};
	
	enum{
		NUM_GLITCH_TYPES = 17,
		MAX_NUM_GLITCHS_ONE_TIME = 5,
	};
	
	/****************************************
	****************************************/
	ofTrueTypeFont font[NUM_FONTSIZE];
	int png_id;
	
	ofVideoGrabber cam;
	int Cam_id;
	
	STATE_TOP StateTop;
	STATE_NOISE StateNoise;
	STATE_REPAIR StateRepair;
	
	int c_CurrentActivePixels;
	int c_TotalActivePixels;
	int c_TotalActivePixels_Last;
	
    ofImage img_Frame;
    ofImage img_Frame_Gray;
    ofImage img_LastFrame_Gray;
    ofImage img_AbsDiff_BinGray;
    ofImage img_BinGray_Cleaned;
    ofImage img_Bin_RGB;
    ofFbo fbo_CamFrame;
    ofFbo fbo_Paint;
    ofFbo fbo_Paint_Back;
	ofFbo fbo_Mask_S;
	ofFbo fbo_Mask_L;
	ofFbo fbo_PreOut;
	ofFbo fbo_Out;
	
	ofPixels pix_Mask_S;
	
	/********************
	********************/
	ofShader shader_AddMask;
	ofShader shader_Mask;
	
	/********************
	********************/
	vector<ofVec2f> ActivePoints_Of_Mask;
	
	/********************
	********************/
	int ArtPaint_id;
	vector<ofImage> ArtPaints;
	vector<int> Order_ArtPaints;
	
	/********************
	********************/
	ofxPostGlitch	myGlitch;
	int Glitch_Ids[NUM_GLITCH_TYPES];
	int NumGlitch_Enable;
	
	/********************
	********************/
	ofSoundPlayer sound_Amb;
	
	ofxSyphonServer SyphonServer;
	
	bool b_flipCamera;
	
	/****************************************
	****************************************/
	void Copy_CamFrame_to_fbo();
	void setup_Gui();
	void setup_Camera();
	void update_img();
	void update_img_OnCam();
	int ForceOdd(int val);
	int Count_ActivePixel(ofPixels& pix, ofColor val);
	void draw_ProcessedImages();
	void StateChart_Top(int now);
	void StateChart_Noise(int now);
	void StateChart_Repair(int now);
	void makeup_image_list(const string dirname, vector<ofImage>& images);
	void inc_ArtPaint_id();
	int getNextId_of_ArtPaint();
	void Refresh_Fbo_ArtPaint();
	void Reset_FboMask();
	
	void Random_Enable_myGlitch();
	void Set_myGlitch(int key, bool b_switch);
	void Clear_AllGlitch();
	
	void Add_DiffArea_To_MaskArea();
	void Fbo_MaskS_to_MaskL();
	void Mask_x_ArtPaint();
	void drawFbo_Preout_to_Out();
	void drawFbo_String_Info_on_Repairing(ofFbo& fbo);
	
	ofVec2f SelectNextTargetToRepair();
	ofVec2f SizeConvert_StoL(ofVec2f from);
	
	void drawFbo_Scale(ofFbo& fbo);
	void drawFbo_Scale_x(ofFbo& fbo, ofVec2f _pos, ofVec2f _scale);
	void drawFbo_Scale_y(ofFbo& fbo, ofVec2f _pos, ofVec2f _scale);
	void drawFbo_ScaleLine_x(ofFbo& fbo);
	void drawFbo_ScaleLine_y(ofFbo& fbo);
	
	void drawFbo_String_TotalActivePixels(ofFbo& fbo);
	void draw_ProcessLine();
	void drawFbo_String_PowerRepairing(ofFbo& fbo);
	
public:
	/****************************************
	****************************************/
	ofApp(int _Cam_id, bool _b_flipCamera);
	~ofApp();

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
