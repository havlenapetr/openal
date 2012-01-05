// sample code for 10/7/04 lecture, simplified version of the demo from OpenAL distribution 

// OpenALDemo.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include "AL/al.h"
#include "AL/alc.h"

void load_test_wav(unsigned int* alSource, unsigned int* alSampleSet)
{
	char*		alBuffer;       // data for the buffer
	ALenum		alFormatBuffer; // buffer format
	ALsizei		alFreqBuffer;   // frequency
	long		alBufferLen;    // bit depth
	ALboolean	alLoop;			// loop
	
	//load the wave file
	alutLoadWAVFile("/sdcard/Barium.wav", &alFormatBuffer, (void **) &alBuffer,
					(unsigned int *)&alBufferLen, &alFreqBuffer, &alLoop);
	
	//create a source
	alGenSources(1, alSource);
	
	//create  buffer
	alGenBuffers(1, alSampleSet);
	
	//put the data into our sampleset buffer
	alBufferData(*alSampleSet, alFormatBuffer, alBuffer, alBufferLen, alFreqBuffer);
	
	//assign the buffer to this source
	alSourcei(*alSource, AL_BUFFER, *alSampleSet);
	
	//release the data
	alutUnloadWAV(alFormatBuffer, alBuffer, alBufferLen, alFreqBuffer);
}

int main()
{
	ALCcontext *context;
	ALCdevice *device;
	unsigned int alSource;
	unsigned int alSampleSet;

	device = alcOpenDevice(NULL);
	if (device == NULL) {
		printf("can't obtain device!\n");
		return -1;
	}

	//Create a context
	context = alcCreateContext(device,NULL);
	if (device == NULL) {
		printf("can't obtain context!\n");
		return -2;
	}

	//Set active context
	alcMakeContextCurrent(context);

	// Clear Error Code
	alGetError();
	
	load_test_wav(&alSource, &alSampleSet);
	
	alSourcei(alSource, AL_LOOPING, AL_TRUE);

	printf("playing . . .\n");
	//play
	alSourcePlay(alSource);

	printf("stopping . . .\n");
	//to stop
	alSourceStop(alSource);

	alDeleteSources(1, &alSource);
	
	//delete our buffer
	alDeleteBuffers(1, &alSampleSet);
	
	context = alcGetCurrentContext();
	
	//Get device for active context
	device = alcGetContextsDevice(context);
	
	//Disable context
	alcMakeContextCurrent(NULL);
	
	//Release context(s)
	alcDestroyContext(context);
	
	//Close device
	alcCloseDevice(device);
	
	printf("end\n");

	return 0;
}