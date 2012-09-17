/*
	File:		QuickTimeAudioUtils.h

	Description: Demonstrates how to use QuickTime APIs to get
				 and manipulate track and movie audio properties.

	Originally introduced at WWDC 2005 at Session 201:
		"Harnessing the Audio Capabilities of QuickTime 7"


	Copyright:   © Copyright 2004, 2005 Apple Computer, Inc.
				 All rights reserved.

	Disclaimer: IMPORTANT:  This Apple software is supplied to you by
	Apple Computer, Inc. ("Apple") in consideration of your agreement to the
	following terms, and your use, installation, modification or
	redistribution of this Apple software constitutes acceptance of these
	terms.  If you do not agree with these terms, please do not use,
	install, modify or redistribute this Apple software.

	In consideration of your agreement to abide by the following terms, and
	subject to these terms, Apple grants you a personal, non-exclusive
	license, under Apple’s copyrights in this original Apple software (the
	"Apple Software"), to use, reproduce, modify and redistribute the Apple
	Software, with or without modifications, in source and/or binary forms;
	provided that if you redistribute the Apple Software in its entirety and
	without modifications, you must retain this notice and the following
	text and disclaimers in all such redistributions of the Apple Software.
	Neither the name, trademarks, service marks or logos of Apple Computer,
	Inc. may be used to endorse or promote products derived from the Apple
	Software without specific prior written permission from Apple.  Except
	as expressly stated in this notice, no other rights or licenses, express
	or implied, are granted by Apple herein, including but not limited to
	any patent rights that may be infringed by your derivative works or by
	other works in which the Apple Software may be incorporated.

	The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
	MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
	THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
	OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.

	IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
	MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
	AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
	STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.

*/
#include "IKAROS_System.h"

#ifdef USE_QUICKTIME_OLD

namespace qt {
#include <AudioToolbox/AudioToolbox.h>
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
}

using namespace qt;


#ifndef fieldOffset
#define fieldOffset(type, field) ((size_t) &((type *) 0)->field)
#endif

extern OSStatus expandChannelLayout(AudioChannelLayout** ioLayout, UInt32* ioLayoutSize);
extern OSStatus getLayoutForTag(AudioChannelLayoutTag tag, UInt32* outLayoutSize, AudioChannelLayout** outLayout);
extern OSStatus getTagForLayout(AudioChannelLayout *inLayout, UInt32 inLayoutSize, AudioChannelLayoutTag *outLayoutTag);
extern OSStatus getTrackLayoutAndSize(Track track,  UInt32* outLayoutSize, AudioChannelLayout** outLayout);
extern OSStatus getDefaultExtractionLayout(Movie movie, UInt32* outLayoutSize, AudioChannelLayout** outLayout, AudioStreamBasicDescription *asbd);
extern OSStatus getDeviceLayout(Movie movie, UInt32* outLayoutSize, AudioChannelLayout** outLayout);
extern OSStatus getDiscreteExtractionLayout(Movie movie, UInt32* outLayoutSize, AudioChannelLayout** outLayout);
extern OSStatus setTrackLayout(Track track,  UInt32 inLayoutSize, AudioChannelLayout* inLayout);
extern Boolean trackMixesToAudioContext(Track track);

extern OSStatus prepareMovieForExtraction(Movie movie,
            MovieAudioExtractionRef* extractionRefPtr,
            Boolean discrete,
            AudioStreamBasicDescription asbd,
            AudioChannelLayout** layout,
            UInt32* layoutSize,
            TimeRecord startTime);
extern OSStatus extractSliceAndScheduleToPlay(MovieAudioExtractionRef extraction,
            AudioStreamBasicDescription asbd,
            AudioUnit playerUnit,
            ScheduledAudioSlice* slice,
            SInt64 *ioNumSamples,
            Boolean *extractionComplete);
extern OSStatus extractAudioToFile(MovieAudioExtractionRef extraction,
                                       AudioFileID outFileID,
                                       AudioStreamBasicDescription asbd,
                                       SInt64 *ioNumSamples,
                                       SInt64* ploc,
                                       Boolean *complete);

#endif
