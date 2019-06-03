/************************************************************
************************************************************/
#include "util.h"

/************************************************************
************************************************************/

/******************************
******************************/
void SJ_UTIL::Array_Forward(int* Array, int Num)
{
	for(int i = 0; i < Num; i++){
		Array[i] = i;
	}
}

/******************************
******************************/
void SJ_UTIL::Array_Forward(vector<int>& Array)
{
	int Num = Array.size();
	
	for(int i = 0; i < Num; i++){
		Array[i] = i;
	}
}

/******************************
******************************/
void SJ_UTIL::Array_Reverse(int* Array, int Num)
{
	for(int i = 0; i < Num; i++){
		Array[i] = Num - 1 - i;
	}
}

/******************************
******************************/
void SJ_UTIL::Array_Reverse(vector<int>& Array)
{
	int Num = Array.size();
	
	for(int i = 0; i < Num; i++){
		Array[i] = Num - 1 - i;
	}
}

/******************************
******************************/
void SJ_UTIL::FisherYates(int* Array, int Num)
{
	/********************
	********************/
	for(int i = 0; i < Num; i++){
		Array[i] = i;
	}

	/********************
	********************/
	int i = Num;
	while(i--){
		/********************
		int j = rand() % (i + 1);
		********************/
		int j = (int)( ((double)rand() / ((double)RAND_MAX + 1)) * (i + 1) );
		
		/********************
		********************/
		int temp = Array[i];
		Array[i] = Array[j];
		Array[j] = temp;
	}
}

/******************************
******************************/
void SJ_UTIL::FisherYates(vector<int>& Array)
{
	int Num = Array.size();
	
	/********************
	********************/
	for(int i = 0; i < Num; i++){
		Array[i] = i;
	}

	/********************
	********************/
	int i = Num;
	while(i--){
		/********************
		int j = rand() % (i + 1);
		********************/
		int j = (int)( ((double)rand() / ((double)RAND_MAX + 1)) * (i + 1) );
		
		/********************
		********************/
		int temp = Array[i];
		Array[i] = Array[j];
		Array[j] = temp;
	}
}

/******************************
******************************/
int SJ_UTIL::Dice_index(int *Weight, int NUM)
{
	/***********************
	cal sum of Weight
	***********************/
	int TotalWeight = 0;
	int i;
	for(i = 0; i < NUM; i++){
		TotalWeight += Weight[i];
	}
	
	if(TotalWeight == 0) { return -1; }
	
	/***********************
	random number
	***********************/
	// int RandomNumber = rand() % TotalWeight;
	int RandomNumber = (int)( ((double)rand() / ((double)RAND_MAX + 1)) * TotalWeight );
	
	/***********************
	define play mode
	***********************/
	int index;
	for(TotalWeight = 0, index = 0; /* */; index++){
		TotalWeight += Weight[index];
		if(RandomNumber < TotalWeight) break;
	}
	
	return index;
}

/******************************
description
	引数"NUM"は、Weightの全体数でなく、確認する数なので、必要.
******************************/
int SJ_UTIL::Dice_index(vector<int>& Weight, int NUM)
{
	/***********************
	***********************/
	if(Weight.size() < NUM){
		Weight.resize(NUM);
	}
	
	/***********************
	cal sum of Weight
	***********************/
	int TotalWeight = 0;
	int i;
	for(i = 0; i < NUM; i++){
		TotalWeight += Weight[i];
	}
	
	if(TotalWeight == 0) { return -1; }
	
	/***********************
	random number
	***********************/
	// int RandomNumber = rand() % TotalWeight;
	int RandomNumber = (int)( ((double)rand() / ((double)RAND_MAX + 1)) * TotalWeight );
	
	/***********************
	define play mode
	***********************/
	int index;
	for(TotalWeight = 0, index = 0; /* */; index++){
		TotalWeight += Weight[index];
		if(RandomNumber < TotalWeight) break;
	}
	
	return index;
}

/******************************
******************************/
void SJ_UTIL::setup_sound(ofSoundPlayer& sound, string FileName, bool b_Loop, float vol)
{
	sound.load(FileName.c_str());
	if(sound.isLoaded()){
		sound.setLoop(b_Loop);
		sound.setMultiPlay(true);
		sound.setVolume(vol);
	}else{
		printf("%s load Error\n", FileName.c_str());
		fflush(stdout);
	}
}



