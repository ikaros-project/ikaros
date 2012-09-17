//
//	InputQTAudio.cc		This file is a part of the IKAROS project
//						A module for reading audio from Quicktime files.
//
//    Copyright (C) 2001-2003  Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//		CGP; 3 August 2005; Created based on InputQTMovie module.
//      CB; 24 July 2009; moved all QT code to data object

#include "InputQTAudio.h"

#ifdef USE_QUICKTIME_OLD

namespace qt {
#include <QuickTime/QuickTime.h>
}

using namespace qt;

#include "QuickTimeAudioUtils.h"
#include "MTAudioBufferListUtils.h"

#define	BailNULL(n)  if (!n) goto bail;
#define	BailError(n) if (n) goto bail;



typedef NavObjectFilterUPP	QTFrameFileFilterUPP;

class InputQTAudioData
{
public:
    qt::Movie                       movie;
    qt::TimeValue                   movieTime;
    qt::UInt32                      m_nextNumberOfSamples; // the number of audio samples in the next visual frame
    qt::UInt32                      m_maxNumSamples; // estimate of the max number of audio samples for a visual frame
    qt::AudioStreamBasicDescription m_summaryASBD;
    qt::MovieAudioExtractionRef     m_audioRef;
    qt::AudioBufferList *           m_audioData;
    qt::UInt32                      m_totalAudioSampleCount;
    qt::UInt32                      m_channelBufferLength;
    int                             m_requestedSamplesPerBuffer;
    qt::TimeValue                   m_movieDuration;
};


//------------------------------------------------------------------------------
//	Open up the Movie file and get a Movie from it.
//------------------------------------------------------------------------------
void
InputQTAudio::OpenMovie(UInt8 * file_name)
{
    FSRef	theFSRef;
    FSSpec	theFSSpec;
    short	refnum = 0;
    Boolean  isDirectory;

    OSErr err = noErr;

    err =FSPathMakeRef (file_name, &theFSRef, &isDirectory);
    BailError(err);

    err =FSGetCatalogInfo (&theFSRef, kFSCatInfoNone, NULL, NULL, &theFSSpec, NULL);
    BailError(err);

    err = OpenMovieFile(&theFSSpec, &refnum, fsRdPerm);
    BailError(err);

    err = NewMovieFromFile(&(data->movie), refnum, NULL, NULL, newMovieActive, NULL);
    BailError(err);

bail:
    if (refnum)
        CloseMovieFile(refnum);
}



unsigned long InputQTAudio::GetNumSamplesInNextFrame()
{
    TimeRecord timeRecord;
    OSType myTypes[1];
    myTypes[0] = VisualMediaCharacteristic;					// we want video samples
    short interestingTime = nextTimeStep;
    static bool endOfMovie = false;

    /*
    If you keep accumulating time from frame to frame, and then extracting audio samples of that duration, 
    then you could drift by a fraction of a sample for each frame, due to the rounding in the UInt32 cast. 
    It's unlikely to be a big problem, but nevertheless since the audio is a continuous stream of uninterrupted samples, 
    it would be better if you used the extracted audio sample count to determine your position.

    For instance, in your application, use GetNextInterestingTime to determine the input movie time of the start of the next video frame. 
    Then extract enough audio samples to get from where you last extracted to that point. 
    Keep a running count of the total number of audio samples you have extracted and the output movie time equals ((Float64) totalAudioSampleCount / asbd.mSampleRate).

    The number of audio samples to extract for the next frame is thus:
    (UInt32) ((((Float64) *(TimeValue64*)&nextInterestingTimeRecord.value / (Float64) nextInterestingTimeRecord.scale) * asbd.mSampleRate) - totalAudioSampleCount).

    Daniel Steinberg
    QuickTime Engineering
    */

    GetMovieNextInterestingTime(data->movie, interestingTime, 1, myTypes, data->movieTime, fixed1, &(data->movieTime), NULL);

    if ((data->movieTime == -1) && (! endOfMovie)) {
        // GetMovieNextInterestingTime is indicating there are no more visual frames, but this may not be the end of the movie. If we use this, we may
        // not obtain all of the audio in the movie. So, use the movie duration as the last movieTime.
        if (data->m_movieDuration > data->movieTime) {
            data->movieTime = data->m_movieDuration;
            printf("InputQTAudio::GetNumSamplesInNextFrame: movieTime == -1, and m_movieDuration > movieTime\n");
        }

        endOfMovie = true; // enter into this conditional only once
    }

    if (data->movieTime != -1)
    {
        int errCode = GetMoviesError();
        if (errCode != noErr)
        {
            Notify(msg_warning, "QuickTime ErrCode = %d (t = %d)\n", errCode, (int) (data->movieTime));
        }

        // Now, set our movie to be positioned at this interesting time, so we can get the TimeRecord
        SetMovieTimeValue(data->movie, data->movieTime);

        // Get the TimeRecord
        GetMovieTime(data->movie, &timeRecord);

//		printf("InputQTAudio::GetNumSamplesInNextFrame: time record: value= %ld, scale= %f\n",  (long)  *((TimeValue64*) &timeRecord.value), (Float64) (timeRecord.scale));

        // Convert to floating-point seconds
        Float64 nextTime = (*((TimeValue64*) &timeRecord.value) / (Float64) timeRecord.scale);

//		printf("InputQTAudio::GetNumSamplesInNextFrame: time at end of next video frame= %f\n", nextTime);

        // This should give me the TOTAL number of audio samples (i.e., samples/sec * seconds = samples) that span from
        // the start of the video until the "nextTime".
        UInt32 totalNumSamples = (UInt32) (data->m_summaryASBD.mSampleRate * nextTime);

//		printf("InputQTAudio::GetNumSamplesInNextFrame: TOTAL Number of samples= %d\n", (int) totalNumSamples);

        // m_totalAudioSampleCount is the total number of audio samples we have obtained from the movie previously.
        UInt32 result = totalNumSamples - data->m_totalAudioSampleCount;

//		printf("InputQTAudio::GetNumSamplesInNextFrame: Number of samples to read next= %d\n", (int) result);

        return result;
    }

    Notify(msg_end_of_file);

    return 0;
}

void InputQTAudio::InitAudio()
{
    OSStatus r;

    AudioChannelLayout* outLayout;

//	printf("InputQTAudio::InitAudio: Initializing audio\n");

    r = getDefaultExtractionLayout(data->movie, NULL, &outLayout, &(data->m_summaryASBD));
    if (r != noErr) {
        printf("InputQTAudio::InitAudio: Error calling getDefaultExtractionLayout\n");
        return;
    }
    /*
    	cout << "mSampleRate= " << m_summaryASBD.mSampleRate << endl;
    	cout << "mFormatID= " << m_summaryASBD.mFormatID << endl;
    	cout << "mFormatFlags= " << m_summaryASBD.mFormatFlags  << endl;
    	cout << "mBytesPerPacket= " << m_summaryASBD.mBytesPerPacket  << endl;
    	cout << "mFramesPerPacket= " << m_summaryASBD.mFramesPerPacket << endl;
    	cout << "mBytesPerFrame= " << m_summaryASBD.mBytesPerFrame << endl;
    	cout << "mChannelsPerFrame= " << m_summaryASBD.mChannelsPerFrame << endl;
    	cout << "mBitsPerChannel= " << m_summaryASBD.mBitsPerChannel << endl;
    	cout << "mReserved= " << m_summaryASBD.mReserved << endl;

    	cout << kAudioFormatLinearPCM << endl <<
    kAudioFormatAC3 << endl <<
    kAudioFormat60958AC3 << endl <<
    kAudioFormatAppleIMA4 << endl <<
    kAudioFormatMPEG4AAC << endl <<
    kAudioFormatMPEG4CELP <<  endl <<
    kAudioFormatMPEG4HVXC << endl <<
    kAudioFormatMPEG4TwinVQ << endl <<
    kAudioFormatTimeCode <<  endl <<
    kAudioFormatMIDIStream << endl <<
    kAudioFormatParameterValueStream << endl;
    */

    // the number of audio channels
//	printf("InputQTAudio::InitAudio: outLayout->mNumberChannelDescriptions= %d\n",  (int) outLayout->mNumberChannelDescriptions);
    m_numChannels = (int) outLayout->mNumberChannelDescriptions;

    r = MovieAudioExtractionBegin(data->movie, 0, &(data->m_audioRef));
    if (r != noErr) {
        printf("InputQTAudio::InitAudio: Error calling MovieAudioExtractionBegin\n");
        return;
    }

    data->m_movieDuration = GetMovieDuration(data->movie);

//	printf("InputQTAudio::InitAudio: Apparent success in initializing audio!\n");

    free(outLayout);
}

InputQTAudio::InputQTAudio(Parameter * p):
        Module(p)
{
    data = new InputQTAudioData();
    
    data->movie = NULL;
    data->movieTime = 0;			// set current time value to begining of the Movie
    data->m_totalAudioSampleCount = 0;

    filename = GetValue("filename");

    if (!filename)
    {
        Notify(msg_fatal_error, "No filename for InputQTAudio.\n");
        return;
    }

    data->m_requestedSamplesPerBuffer  = GetIntValue("samples_per_buffer", -1);
    if (data->m_requestedSamplesPerBuffer <= 0) {
        // Just in case the user gives us a strange value
        data->m_requestedSamplesPerBuffer = -1;
    }

    // enables the user to select only the first n of the audio channels
    m_numChannelsRequested = GetIntValue("num_channels_requested", -1);

//	printf("InputQTAudio::InputQTAudio: m_requestedSamplesPerBuffer= %d\n", m_requestedSamplesPerBuffer); fflush(stdout);

    OSErr result = noErr;

    // This seems to be Mac GUI related-- to make the cursor visible
    InitCursor();

    // Initialize the QuickTime Movie Toolbox
    result = EnterMovies();
    if (result != noErr)
        return;

    // get the movie
    OpenMovie((UInt8 *)filename);
    if (!data->movie) {
        printf("InputQTAudio: Cannot open movie for filename= %s\n", filename);
        Notify(msg_fatal_error, "InputQTAudio: Cannot open movie for filename= %s\n", filename);
        return;
    }

    // Initialize audio
    InitAudio();

    data->m_audioData = NULL;

    if (data->m_requestedSamplesPerBuffer == -1) {
        // User did not request a number of samples per buffer, so synchronize audio buffers with the visual track.

        data->m_nextNumberOfSamples = GetNumSamplesInNextFrame();

        // Estimate the maximum number of samples as 2 * the number of audio samples associated with the first visual frame.
        // I have no idea if this is the max, but I don't have a better way to estimate the max right now.
        data->m_maxNumSamples = data->m_nextNumberOfSamples * 2;
    } else {
        // User did request a number of samples per buffer
        data->m_nextNumberOfSamples = data->m_requestedSamplesPerBuffer;
        data->m_maxNumSamples = data->m_requestedSamplesPerBuffer;
    }

    if ((m_numChannelsRequested != -1) && (m_numChannelsRequested > 0) && (m_numChannelsRequested < m_numChannels)) {
        AddOutput("CHANNELS", m_numChannelsRequested, data->m_maxNumSamples);
    } else {
        m_numChannelsRequested = m_numChannels;
        // This output will contain all of the audio channels. Each row will correspond to one audio frame from all channels.
        // Each column will contain data for a single channel. We attempt to allocate the maximum possible number of rows for this output.
        AddOutput("CHANNELS", m_numChannels, data->m_maxNumSamples);
    }

    // This output will contain a single number on each tick-- the number of rows that have values assigned in the CHANNELS output.
    AddOutput("NUMBER_SAMPLES", 1, 1);

    // Need to allocate the AudioBufferList in order to call MovieAudioExtractionFillBuffer.
    // The documentation indicates that allocation of an AudioBufferList varies depending on whether the audio
    // is interleaved. At this point, I'm assuming that the interleaved option is not specifying how the audio is represented in the file, but
    // rather how the user wants to represent the audio. I.e., I'm assuming that the MovieAudioExtractionFillBuffer call can fill up the
    // AudioBufferList in either a interleaved or non-interleaved fashion.
    data->m_audioData = MTAudioBufferListNew (m_numChannels, data->m_maxNumSamples, false);
    if (data->m_audioData == NULL) {
        Notify(msg_fatal_error, "InputQTAudio::InputQTAudio: Error allocating AudioBufferList\n");
        return;
    }

    // For some ungodly reason MovieAudioExtractionFillBuffer alters the mDataByteSize when called, so this needs to be reset before
    // calling MovieAudioExtractionFillBuffer the second, third etc. times.
    data->m_channelBufferLength = data->m_audioData->mBuffers[0].mDataByteSize;
}



InputQTAudio::~InputQTAudio()
{
    if (data->movie)
        DisposeMovie(data->movie);

    if (data->m_audioData != NULL) {
        MTAudioBufferListDispose (data->m_audioData);
    }
    
    delete data;
}



Module *
InputQTAudio::Create(Parameter * p)
{
    return new InputQTAudio(p);
}



void
InputQTAudio::Init()
{
    Notify(msg_warning, "InputQTAudio is not thread safe\n");

    channels = GetOutputArray("CHANNELS");
    numberSamples = GetOutputArray("NUMBER_SAMPLES");
}

FILE *fp = NULL;

void
InputQTAudio::Tick()
{
    UInt32 audioFlags;
    OSStatus r;

    r = MovieAudioExtractionFillBuffer( data->m_audioRef,	// our movie audio extract. session ref
                                        &(data->m_nextNumberOfSamples),	// number of PCM frames to be extracted
                                        data->m_audioData,		// AudioBufferList
                                        &audioFlags);	// tells us when extraction is complete

    data->m_totalAudioSampleCount += data->m_nextNumberOfSamples;

    if (r != noErr) {
        printf("InputQTAudio::Tick: Error calling MovieAudioExtractionFillBuffer\n");
        printf("InputQTAudio::Tick: numSamples= %d, audioFlags= %d\n", (int) (data->m_nextNumberOfSamples), (int) audioFlags);
        return;
    }

    // As noted in the .h file, for some ungodly reason, MovieAudioExtractionFillBuffer alters the mDataByteSize field, so we need to reset it.
    for (unsigned channelIndex=0; channelIndex < data->m_audioData->mNumberBuffers; channelIndex++) {
        data->m_audioData->mBuffers[channelIndex].mDataByteSize = data->m_channelBufferLength;
    }

    printf("InputQTAudio::Tick: numSamples= %d, audioFlags= %d\n", (int) (data->m_nextNumberOfSamples), (int) audioFlags);

    if ((audioFlags == kQTMovieAudioExtractionComplete) && (data->m_nextNumberOfSamples == 0)) {
        printf("InputQTAudio::Tick: End of file\n");
        Notify(msg_end_of_file);
    }

    // Copy to IKAROS output arrays
    *numberSamples = (Float32) (data->m_nextNumberOfSamples);
    Float32 *output = channels;

    /*
    if (fp == NULL) {
    	fp = fopen("/tmp/debug2.out", "w");
    }
    */

    for (unsigned i=0; i < data->m_nextNumberOfSamples; i++) {
        //printf("t4- s= %d: ", (int) i);
        for (unsigned channelIndex = 0; channelIndex < (unsigned) m_numChannelsRequested; channelIndex++) {
            *output = ((Float32 *) (data->m_audioData->mBuffers[channelIndex].mData))[i];
            //printf("c[%d]= %f; ", (int) channelIndex, *output);
            output++;
        }

        //fprintf(fp, "\n"); fflush(fp);
    }

    //printf("\n");

    if (data->m_requestedSamplesPerBuffer == -1) {
        data->m_nextNumberOfSamples = GetNumSamplesInNextFrame();

        // Only need to make this check if we are synchronizing with the visual track-- otherwise, we should be reading exactly the number of samples that the user requested
        if (data->m_nextNumberOfSamples > data->m_maxNumSamples) {
            printf("InputQTAudio::Tick: maximum number of audio samples exceeded: %d (max: %d)\n", (int) (data->m_nextNumberOfSamples), (int) (data->m_maxNumSamples));
            Notify(msg_fatal_error, "InputQTAudio::Tick: maximum number of audio samples exceeded: %d (max: %d)\n", (int) (data->m_nextNumberOfSamples), (int) (data->m_maxNumSamples));
            return; // shouldn't get to here
        }
    }
}

#endif
