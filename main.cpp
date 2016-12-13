/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../lib-src/fmod/api/inc/fmod.hpp"
#include "../lib-src/fmod/api/inc/fmod_errors.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>

//The event signaled when the app should be terminated.
HANDLE g_hTerminateEvent = NULL;
//Handles events that would normally terminate a console application. 
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);


void ERRCHECK(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}
//   do            re            mi     fa            sol           la            si
static const char *note[120] =
{
    "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",  
    "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",  
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",  
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",  
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",  
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",  
    "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",  
    "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",  
    "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8",  
    "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9", "G#9", "A9", "A#9", "B9"
};

static const float notefreq[120] =
{
      16.35f,   17.32f,   18.35f,   19.45f,    20.60f,    21.83f,    23.12f,    24.50f,    25.96f,    27.50f,    29.14f,    30.87f, 
      32.70f,   34.65f,   36.71f,   38.89f,    41.20f,    43.65f,    46.25f,    49.00f,    51.91f,    55.00f,    58.27f,    61.74f, 
      65.41f,   69.30f,   73.42f,   77.78f,    82.41f,    87.31f,    92.50f,    98.00f,   103.83f,   110.00f,   116.54f,   123.47f, 
     130.81f,  138.59f,  146.83f,  155.56f,   164.81f,   174.61f,   185.00f,   196.00f,   207.65f,   220.00f,   233.08f,   246.94f, 
     261.63f,  277.18f,  293.66f,  311.13f,   329.63f,   349.23f,   369.99f,   392.00f,   415.30f,   440.00f,   466.16f,   493.88f, 
     523.25f,  554.37f,  587.33f,  622.25f,   659.26f,   698.46f,   739.99f,   783.99f,   830.61f,   880.00f,   932.33f,   987.77f, 
    1046.50f, 1108.73f, 1174.66f, 1244.51f,  1318.51f,  1396.91f,  1479.98f,  1567.98f,  1661.22f,  1760.00f,  1864.66f,  1975.53f, 
    2093.00f, 2217.46f, 2349.32f, 2489.02f,  2637.02f,  2793.83f,  2959.96f,  3135.96f,  3322.44f,  3520.00f,  3729.31f,  3951.07f, 
    4186.01f, 4434.92f, 4698.64f, 4978.03f,  5274.04f,  5587.65f,  5919.91f,  6271.92f,  6644.87f,  7040.00f,  7458.62f,  7902.13f, 
    8372.01f, 8869.84f, 9397.27f, 9956.06f, 10548.08f, 11175.30f, 11839.82f, 12543.85f, 13289.75f, 14080.00f, 14917.24f, 15804.26f
};

#define OUTPUTRATE          48000
//#define OUTPUTRATE          44100
#define SPECTRUMSIZE        8192
#define SPECTRUMRANGE       ((float)OUTPUTRATE / 2.0f)      /* 0 to nyquist */

#define BINSIZE      (SPECTRUMRANGE / (float)SPECTRUMSIZE)

FMOD::System* global_pSystem=NULL;
FMOD::Sound* global_pSound=NULL;
int Terminate();
int main(int argc, char *argv[])
{
	//2012june22, spi, begin
	char wavfilename[2048] = {"a#4.wav"};
	char txtfilename[2048] = {""};
	float maxdetectionduration_s = 1.0f; //if negative, detection will last the duration of the sound file
	if(argc>1)
	{
		strcpy(wavfilename, argv[1]);
	}
	if(argc>2)
	{
		maxdetectionduration_s = atof(argv[2]);
	}
	strcpy(txtfilename, wavfilename);
	char* pChar = strrchr(txtfilename,'.');
	strcpy(&(pChar[1]), "txt");
	//Auto-reset, initially non-signaled event 
    g_hTerminateEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    //Add the break handler
    ::SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	//2012june22, spi, end

    //FMOD::System* system  = 0;
    //FMOD::Sound *sound   = 0;
    FMOD::Channel* channel = 0;
    FMOD_RESULT result;
    FMOD_CREATESOUNDEXINFO exinfo;
    int key, driver, recorddriver, numdrivers, count, bin;
    unsigned int version;    

    /*
        Create a System object and initialize.
    */
	FMOD::Debug_SetLevel(FMOD_DEBUG_ALL);
    result = FMOD::System_Create(&global_pSystem);
    ERRCHECK(result);
	if(result!=FMOD_OK) 
	{
		Terminate();
		return 1;
	}

    result = global_pSystem->getVersion(&version);
    ERRCHECK(result);
	if(result!=FMOD_OK) 
	{
		Terminate();
		return 1;
	}

    if (version < FMOD_VERSION)
    {
        printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
        return 0;
    }

    ///////////////////////// 
    //  System initialization
    /////////////////////////
	//2012june22, spi, end
	/*
    printf("---------------------------------------------------------\n");    
    printf("Select OUTPUT type\n");    
    printf("---------------------------------------------------------\n");    
    printf("1 :  DirectSound\n");
    printf("2 :  Windows Multimedia WaveOut\n");
    printf("3 :  ASIO\n");
    printf("---------------------------------------------------------\n");
    printf("Press a corresponding number or ESC to quit\n");

    do
    {
        key = _getch();
    } while (key != 27 && key < '1' && key > '5');
    
    switch (key)
    {
        case '1' :  result = system->setOutput(FMOD_OUTPUTTYPE_DSOUND);
                    break;
        case '2' :  result = system->setOutput(FMOD_OUTPUTTYPE_WINMM);
                    break;
        case '3' :  result = system->setOutput(FMOD_OUTPUTTYPE_ASIO);
                    break;
        default  :  return 1; 
    }  
    ERRCHECK(result);
    */
	printf("selecting output to DirectSound\n");
	result = global_pSystem->setOutput(FMOD_OUTPUTTYPE_DSOUND);
    ERRCHECK(result);
	if(result!=FMOD_OK) 
	{
		Terminate();
		return 1;
	}
	//2012june22, spi, end


    ////////////////////////////////
    //    Enumerate playback devices
    ////////////////////////////////
	//2012june22, spi, begin
	/*
    result = system->getNumDrivers(&numdrivers);
    ERRCHECK(result);

    printf("---------------------------------------------------------\n");    
    printf("Choose a PLAYBACK driver\n");
    printf("---------------------------------------------------------\n");    
    for (count=0; count < numdrivers; count++)
    {
        char name[256];

        result = system->getDriverInfo(count, name, 256, 0);
        ERRCHECK(result);

        printf("%d : %s\n", count + 1, name);
    }
    printf("---------------------------------------------------------\n");
    printf("Press a corresponding number or ESC to quit\n");

    do
    {
        key = _getch();
        if (key == 27)
        {
            return 0;
        }
        driver = key - '1';
    } while (driver < 0 || driver >= numdrivers);

    result = system->setDriver(driver);
    ERRCHECK(result);
	*/
	driver = 0;
    result = global_pSystem->setDriver(driver);
    ERRCHECK(result);
	if(result!=FMOD_OK) 
	{
		Terminate();
		return 1;
	}
	//2012june22, spi, end


    //////////////////////////////
    //    Enumerate record devices
    //////////////////////////////
	//2012june22, spi, begin
	/*
    result = system->getRecordNumDrivers(&numdrivers);
    ERRCHECK(result);

    printf("---------------------------------------------------------\n");    
    printf("Choose a RECORD driver\n");
    printf("---------------------------------------------------------\n");    
    for (count=0; count < numdrivers; count++)
    {
        char name[256];

        result = system->getRecordDriverInfo(count, name, 256, 0);
        ERRCHECK(result);

        printf("%d : %s\n", count + 1, name);
    }
    printf("---------------------------------------------------------\n");
    printf("Press a corresponding number or ESC to quit\n");

    recorddriver = 0;
    do
    {
        key = _getch();
        if (key == 27)
        {
            return 0;
        }
        recorddriver = key - '1';
    } while (recorddriver < 0 || recorddriver >= numdrivers);

    printf("\n");
	*/
	recorddriver=0;
	//2012june22, spi, end

    result = global_pSystem->setSoftwareFormat(OUTPUTRATE, FMOD_SOUND_FORMAT_PCM16, 1, 0, FMOD_DSP_RESAMPLER_LINEAR);
    ERRCHECK(result);
	if(result!=FMOD_OK) 
	{
		Terminate();
		return 1;
	}
    result = global_pSystem->init(32, FMOD_INIT_NORMAL, 0);
    ERRCHECK(result);
	if(result!=FMOD_OK) 
	{
		Terminate();
		return 1;
	}

	/*
    //
    //  Create a sound to record to.
    //
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));

    exinfo.cbsize           = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels      = 1;
    exinfo.format           = FMOD_SOUND_FORMAT_PCM16;
    exinfo.defaultfrequency = OUTPUTRATE;
    exinfo.length           = exinfo.defaultfrequency * sizeof(short) * exinfo.numchannels * 5;
    
    result = system->createSound(0, FMOD_2D | FMOD_SOFTWARE | FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &sound);
    ERRCHECK(result);
	*/

    //
    //  Start the interface
    //
    printf("===============================================================\n");
    printf("Pitch detection example.  Copyright (c) nakedsoftware.org 2012.\n");
    printf("===============================================================\n");
    printf("\n");
    /*
	printf("Record something through the selected recording device and FMOD will\n");
    printf("Determine the pitch.  Sustain the tone for at least a second to get an\n");
    printf("accurate reading.\n");
	*/
	printf("Drag and drop a wav file over spipitchdetection.exe and FMOD will\n");
    printf("determine the pitch.  Sustain the tone for at least a second to get an\n");
    printf("accurate reading.\n");
    printf("Press 'Esc' to quit\n");
    printf("\n");

	/*
    result = system->recordStart(recorddriver, sound, true);
    ERRCHECK(result);
  
    Sleep(200);      //Give it some time to record something 
    */

	//2012june22, spi, begin
	/*
    result = system->playSound(FMOD_CHANNEL_REUSE, sound, false, &channel);
    ERRCHECK(result);
    // Dont hear what is being recorded otherwise it will feedback.  Spectrum analysis is done before volume scaling in the DSP chain
    result = channel->setVolume(0);
    ERRCHECK(result);
	*/
	//result = system->createSound("a4.wav", FMOD_HARDWARE, 0, &pSound);
	//result = system->createSound("a0.wav", FMOD_HARDWARE, 0, &pSound);
	//result = system->createSound("c3.wav", FMOD_HARDWARE, 0, &pSound);
	//result = system->createSound("a#4.wav", FMOD_HARDWARE, 0, &pSound);
	result = global_pSystem->createSound(wavfilename, FMOD_HARDWARE, 0, &global_pSound);
    ERRCHECK(result);
	if(result!=FMOD_OK) 
	{
		Terminate();
		return 1;
	}
	DWORD starttime_ms = GetTickCount(); //log start time
    result = global_pSystem->playSound(FMOD_CHANNEL_REUSE, global_pSound, false, &channel);
    ERRCHECK(result);
	if(result!=FMOD_OK) 
	{
		Terminate();
		return 1;
	}
	//2012june22, spi, end

	//erase the file if it exists
	FILE* pFILE=fopen(txtfilename, "w"); 
	if(pFILE!=NULL)
	{
		fclose(pFILE);
	}

    bin = 0;

    /*
        Main loop.
    */
    do
    {
        static float spectrum[SPECTRUMSIZE];
        float        dominanthz = 0;
        float        max;
        int          dominantnote = 0;
        float        binsize = BINSIZE;

        if (_kbhit())
        {
            key = _getch();
        }

        result = channel->getSpectrum(spectrum, SPECTRUMSIZE, 0, FMOD_DSP_FFT_WINDOW_TRIANGLE);
        ERRCHECK(result);
		if(result!=FMOD_OK) 
		{
			Terminate();
			return 1;
		}

        max = 0;

        for (count = 0; count < SPECTRUMSIZE; count++)
        {
            if (spectrum[count] > 0.01f && spectrum[count] > max)
            {
                max = spectrum[count];
                bin = count;
            }
        }        

        dominanthz  = (float)bin * BINSIZE;       /* dominant frequency min */

        dominantnote = 0;
        for (count = 0; count < 120; count++)
        {
             if (dominanthz >= notefreq[count] && dominanthz < notefreq[count + 1])
             {
                /* which is it closer to.  This note or the next note */
                if (fabs(dominanthz - notefreq[count]) < fabs(dominanthz - notefreq[count+1]))
                {
                    dominantnote = count;
                }
                else
                {
                    dominantnote = count + 1;
                }
                break;
             }
        }

        printf("Detected rate : %7.1f -> %7.1f hz. Detected musical note. %-3s (%7.1f hz).\r", dominanthz, ((float)bin + 0.99f) * BINSIZE, note[dominantnote], notefreq[dominantnote]);
		//2012june22, spi, begin
		FILE* pFILE=fopen(txtfilename, "w"); //unique line
		//FILE* pFILE=fopen(txtfilename, "a"); //multiple lines
		if(pFILE!=NULL)
		{
			fprintf(pFILE, "%f %s %d\n", notefreq[dominantnote], note[dominantnote], dominantnote+12);
			fclose(pFILE);
			//key=27;//to make it end right away
		}
#ifdef DEBUG 
#else
		//for release only
		DWORD currenttime_ms = GetTickCount();
		float totalduration_s = (currenttime_ms-starttime_ms)/1000.0;
		if(maxdetectionduration_s>0.0f && totalduration_s>maxdetectionduration_s)
		{
			key=27;//to make it end right away
		}
#endif
		//2012june22, spi, end
        global_pSystem->update();

        Sleep(10);

    } while (key != 27);

    printf("\n");

    //
    //   Shut down
    //
	//2012june22, spi, begin
	//done, write an empty file named spd_ok.txt in order to tell other applications this process is terminated. 
	//note, prior to launching this process, other applications should be deleting this sps_ok.txt file and wait for it to appear. 
	FILE* pFILE2=fopen("spd_ok.txt", "w");
	if(pFILE2!=NULL)
	{
		fclose(pFILE2);
	}

	/*
    result = sound->release();
    ERRCHECK(result);
    result = system->release();
    ERRCHECK(result);
    return 0;
	*/
    return Terminate();
	//2012june22, spi, end
}

int Terminate()
{
    FMOD_RESULT result;

    result = global_pSound->release();
    ERRCHECK(result);

    result = global_pSystem->release();
    ERRCHECK(result);
	return 0;
}

//Called by the operating system in a separate thread to handle an app-terminating event. 
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT ||
        dwCtrlType == CTRL_BREAK_EVENT ||
        dwCtrlType == CTRL_CLOSE_EVENT)
    {
        // CTRL_C_EVENT - Ctrl+C was pressed 
        // CTRL_BREAK_EVENT - Ctrl+Break was pressed 
        // CTRL_CLOSE_EVENT - Console window was closed 
		Terminate();
        // Tell the main thread to exit the app 
        ::SetEvent(g_hTerminateEvent);
        return TRUE;
    }

    //Not an event handled by this function.
    //The only events that should be able to
	//reach this line of code are events that
    //should only be sent to services. 
    return FALSE;
}
