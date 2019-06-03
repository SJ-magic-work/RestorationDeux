/************************************************************
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"
#include "sj_common.h"

/************************************************************
************************************************************/

class SJ_UTIL{
private:

public:
	static void Array_Forward(int* Array, int Num);
	static void Array_Forward(vector<int>& Array);
	static void Array_Reverse(int* Array, int Num);
	static void Array_Reverse(vector<int>& Array);
	static void FisherYates(int* Array, int Num);
	static void FisherYates(vector<int>& Array);
	
	static int Dice_index(int *Weight, int NUM);
	static int Dice_index(vector<int>& Weight, int NUM);
	
	static void setup_sound(ofSoundPlayer& sound, string FileName, bool b_Loop, float vol);
};
