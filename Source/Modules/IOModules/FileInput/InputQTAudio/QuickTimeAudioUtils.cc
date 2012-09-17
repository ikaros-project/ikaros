/*
	File:		QuickTimeAudioUtils.c

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

#include "IKAROS.h"

#ifdef USE_QUICKTIME_OLD

#include "QuickTimeAudioUtils.h"


// This function takes in an AudioChannelLayout and expands it
// into a full layout with channel descriptions for each channel.
// The 'layout' is passed by reference, and may be reallocated
// to make sure it is large enough to hold the resulting data.
OSStatus expandChannelLayout(AudioChannelLayout** ioLayout, UInt32* ioLayoutSize)
{
    OSStatus			err = noErr;
    AudioChannelLayout	*srcLayout = NULL;
    AudioChannelLayout	*fullLayout = NULL;
    UInt32				newLayoutSize = 0;
    UInt32				size;

    if (!ioLayout || !*ioLayout || !ioLayoutSize)
    {
        err = paramErr;
        goto bail;
    }
    srcLayout = *ioLayout;

    switch (srcLayout->mChannelLayoutTag)
    {
    case kAudioChannelLayoutTag_UseChannelDescriptions:
        // nothing to do, we already have an expanded layout
        break;

    case kAudioChannelLayoutTag_UseChannelBitmap:
        // Get the size of the expanded bitmap layout and allocate a new layout, if necessary
        err = AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForBitmap,
                                         sizeof(UInt32), &srcLayout->mChannelBitmap, &size);
        if (size > *ioLayoutSize)
        {
            fullLayout = (AudioChannelLayout *) calloc(1, size);
            if (fullLayout == nil)
            {
                err = memFullErr;
                goto bail;
            }
            newLayoutSize = size;		// flag that we allocated a new layout
        }
        else
        {
            fullLayout = srcLayout;		// expand in place
        }
        err = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForBitmap,
                                     sizeof(UInt32), &srcLayout->mChannelBitmap, &size, fullLayout);
        if (err)
            goto bail;
        break;

    case kAudioChannelLayoutTag_Mono:
        // The Mono layout tag expands to "Center", which isn't really what we want here.
        // So just expand it manually.
        size = sizeof (AudioChannelLayout);
        srcLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
        srcLayout->mNumberChannelDescriptions = 1;
        srcLayout->mChannelDescriptions[0].mChannelLabel = kAudioChannelLabel_Mono;
        break;

    default: // use specific layout tag
        // Get the size of the expanded bitmap layout and allocate a new layout, if necessary
        err = AudioFormatGetPropertyInfo(kAudioFormatProperty_ChannelLayoutForTag,
                                         sizeof(AudioChannelLayoutTag), &srcLayout->mChannelLayoutTag, &size);
        if (!err & (size > *ioLayoutSize))
        {
            fullLayout = (AudioChannelLayout *) calloc(1, size);
            if (fullLayout == nil)
            {
                err = memFullErr;
                goto bail;
            }
            newLayoutSize = size;		// flag that we allocated a new layout
        }
        else
        {
            fullLayout = srcLayout;		// expand in place
        }
        err = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutForTag, sizeof(AudioChannelLayoutTag),
                                     &srcLayout->mChannelLayoutTag, &size, fullLayout);
        if (err)
            goto bail;
        // The resulting layout still has the layout tag.
        // Change it to UseChannelDescriptions here to avoid confusion elsewhere.
        fullLayout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
        break;
    }

bail:
    if (err == noErr)
    {
        if (fullLayout)
        {
            // Return the expanded layout
            *ioLayout = fullLayout;

            // If a new layout was allocated, return its size and release the old one
            if (newLayoutSize != 0)
            {
                *ioLayoutSize = newLayoutSize;
                free(srcLayout);
            }
        }
    }
    else	// if (err)
    {
        // If a new layout was allocated, free it on error and leave the original layout unchanged
        if (newLayoutSize != 0)
            free(fullLayout);
    }

    return (err);
}

// Get the track's channel layout, expanded into individual channel descriptions.
// The channel layout returned by this function must be deallocated by the client.
// 'outLayoutSize' may be nil.
OSStatus getTrackLayoutAndSize(Track track,  UInt32* outLayoutSize, AudioChannelLayout** outLayout)
{
    OSStatus			err = noErr;
    AudioChannelLayout	*layout = NULL;
    UInt32				size = 0;

    if (!outLayout)
    {
        err = paramErr;
        goto bail;
    }
    // Get the size of the track layout
    err = QTGetTrackPropertyInfo(track, kQTPropertyClass_Audio,kQTAudioPropertyID_ChannelLayout,
                                 NULL, &size, NULL);
    if (err || (size==0))
        goto bail;

    // Allocate memory for the track layout
    layout = (AudioChannelLayout*) calloc(1, size);
    if (layout == nil)
    {
        err = memFullErr;
        goto bail;
    }
    // Get the layout from the track
    err = QTGetTrackProperty(track, kQTPropertyClass_Audio, kQTAudioPropertyID_ChannelLayout,
                             size, layout, NULL);
    if (err)
        goto bail;

    // Expand the layout into a full layout using channel descriptions
    err = expandChannelLayout(&layout, &size);
    if (err)
        goto bail;

    // Return the layout and size, if requested
    *outLayout = layout;
    if (outLayoutSize)
        *outLayoutSize = size;
bail:
    if (err != noErr)
    {
        if (layout)
            free(layout);
        if (outLayoutSize)
            *outLayoutSize = 0;
    }

    return err;
}


// Get the default extraction layout for this movie, expanded into individual channel descriptions.
// The channel layout returned by this routine must be deallocated by the client.
// If 'asbd' is non-NULL, fill it with the default extraction asbd, which contains the
// highest sample rate among the sound tracks that will be contributing.
//	'outLayoutSize' and 'asbd' may be nil.
OSStatus getDefaultExtractionLayout(Movie movie, UInt32* outLayoutSize,
                                    AudioChannelLayout** outLayout, AudioStreamBasicDescription *asbd)
{
    OSStatus			err = noErr;
    AudioChannelLayout *layout = NULL;
    UInt32				size = 0;
    MovieAudioExtractionRef	extractionSessionRef = nil;

    if (!outLayout)
    {
        err = paramErr;
        goto bail;
    }

    // Initiate a dummy audio extraction here, in order to get the resulting default channel layout.
    err = MovieAudioExtractionBegin(movie, 0, &extractionSessionRef);
    if (err)
        goto bail;

    // Get the size of the extraction output layout
    err = MovieAudioExtractionGetPropertyInfo(extractionSessionRef, kQTPropertyClass_MovieAudioExtraction_Audio,
            kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
            NULL, &size, NULL);
    if (err)
        goto bail;

    // Allocate memory for the layout
    layout = (AudioChannelLayout *) calloc(1, size);
    if (layout == nil)
    {
        err = memFullErr;
        goto bail;
    }

    // Get the layout for the current extraction configuration.
    // This will have already been expanded into channel descriptions.
    err = MovieAudioExtractionGetProperty(extractionSessionRef, kQTPropertyClass_MovieAudioExtraction_Audio,
                                          kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
                                          size, layout, nil);
    if (err)
        goto bail;

    // Return the layout and size, if requested
    *outLayout = layout;
    if (outLayoutSize)
        *outLayoutSize = size;

    // If the ASBD was requested, get that also.
    if (asbd != nil)
    {
        // Get the layout for the current extraction configuration.
        // This will have already been expanded into channel descriptions.
        size = sizeof (*asbd);
        err = MovieAudioExtractionGetProperty(extractionSessionRef,
                                              kQTPropertyClass_MovieAudioExtraction_Audio,
                                              kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
                                              size, asbd, nil);
        if (err)
            goto bail;
    }

bail:
    if (err != noErr)
    {
        if (layout)
            free(layout);
        if (outLayoutSize)
            *outLayoutSize = 0;
    }

    // Throw away the temporary audio extraction session
    if (extractionSessionRef)
        (void) MovieAudioExtractionEnd(extractionSessionRef);

    return err;
}

// Get the output layout for an "All Channels Discrete" audio extraction,
// expanded into individual channel descriptions.
// The channel layout returned by this routine must be deallocated by the client.
// 'outLayoutSize' may be nil.
OSStatus getDiscreteExtractionLayout(Movie movie, UInt32* outLayoutSize, AudioChannelLayout** outLayout)
{
    OSStatus			err = noErr;
    Boolean				discrete = true;
    AudioChannelLayout *layout = NULL;
    UInt32				size = 0;
    MovieAudioExtractionRef	extractionSessionRef = nil;

    if (!outLayout)
    {
        err = paramErr;
        goto bail;
    }

    // Initiate a dummy audio extraction here, in order to get the resulting discrete channel layout.
    err = MovieAudioExtractionBegin(movie, 0, &extractionSessionRef);
    if (err)
        goto bail;

    // Set the 'all channels discrete' property
    err = MovieAudioExtractionSetProperty(extractionSessionRef,
                                          kQTPropertyClass_MovieAudioExtraction_Movie,
                                          kQTMovieAudioExtractionMoviePropertyID_AllChannelsDiscrete,
                                          sizeof (discrete),
                                          &discrete);
    if (err)
        goto bail;

    // Get the size of the extraction output layout
    err = MovieAudioExtractionGetPropertyInfo(extractionSessionRef, kQTPropertyClass_MovieAudioExtraction_Audio,
            kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
            NULL, &size, NULL);
    if (err)
        goto bail;

    // Allocate memory for the layout
    layout = (AudioChannelLayout *) calloc(1, size);
    if (layout == nil)
    {
        err = memFullErr;
        goto bail;
    }

    // Get the layout for the current extraction configuration.
    // This will have already been expanded into channel descriptions.
    err = MovieAudioExtractionGetProperty(extractionSessionRef,
                                          kQTPropertyClass_MovieAudioExtraction_Audio,
                                          kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
                                          size, layout, nil);
    if (err)
        goto bail;

    // Return the layout and size, if requested
    *outLayout = layout;
    if (outLayoutSize)
        *outLayoutSize = size;
bail:
    if (err != noErr)
    {
        if (layout)
            free(layout);
        if (outLayoutSize)
            *outLayoutSize = 0;
    }

    // Throw away the temporary audio extraction session
    if (extractionSessionRef)
        (void) MovieAudioExtractionEnd(extractionSessionRef);

    return err;
}

// Get the channel layout for the audio device that this movie is playing to,
// expanded into individual channel descriptions.
// The channel layout returned by this routine must be deallocated by the client.
// 'outLayoutSize' may be nil.
OSStatus getDeviceLayout(Movie movie, UInt32* outLayoutSize, AudioChannelLayout** outLayout)
{
    OSStatus			err = noErr;
    AudioChannelLayout	*layout = NULL;
    UInt32				size = 0;

    if (!outLayout)
    {
        err = paramErr;
        goto bail;
    }

    // Get the size of the device layout
    err = QTGetMoviePropertyInfo(movie, kQTPropertyClass_Audio, kQTAudioPropertyID_DeviceChannelLayout,
                                 NULL, &size, NULL);

    if (err || (size==0))
        goto bail;

    // Allocate memory for the device layout
    layout = (AudioChannelLayout*) calloc(1, size);
    if (layout == nil)
    {
        err = memFullErr;
        goto bail;
    }

    // Get the device layout from the movie
    err = QTGetMovieProperty(movie,
                             kQTPropertyClass_Audio,
                             kQTAudioPropertyID_DeviceChannelLayout,
                             size,
                             layout,
                             NULL);
    if (err)
        goto bail;

    // Expand the layout into a full layout using channel descriptions
    err = expandChannelLayout(&layout, &size);
    if (err)
        goto bail;

    // Return the layout and size, if requested
    *outLayout = layout;
    if (outLayoutSize)
        *outLayoutSize = size;
bail:
    if (err != noErr)
    {
        if (layout)
            free(layout);
        if (outLayoutSize)
            *outLayoutSize = 0;
    }

    return err;

}

// Get the channel layout for a specified channel layout tag.
// The channel layout returned by this routine must be deallocated by the client.
// 'outLayoutSize' may be nil.
OSStatus getLayoutForTag(AudioChannelLayoutTag layoutTag, UInt32* outLayoutSize, AudioChannelLayout** outLayout)
{
    OSStatus			err = noErr;
    AudioChannelLayout *layout = NULL;
    UInt32				size = 0;
    UInt32				channels;

    if (!outLayout)
    {
        err = paramErr;
        goto bail;
    }

    // Allocate memory for the channel layout
    channels = AudioChannelLayoutTag_GetNumberOfChannels(layoutTag);
    size = fieldOffset(AudioChannelLayout, mChannelDescriptions[channels]);
    layout = (AudioChannelLayout*) calloc(1, size);
    if (layout == nil)
    {
        err = memFullErr;
        goto bail;
    }

    // Expand the layout into a full layout using channel descriptions
    layout->mChannelLayoutTag = layoutTag;
    err = expandChannelLayout(&layout, &size);
    if (err)
        goto bail;

    // Return the layout and size, if requested
    *outLayout = layout;
    if (outLayoutSize)
        *outLayoutSize = size;
bail:
    if (err != noErr)
    {
        if (layout)
            free(layout);
        if (outLayoutSize)
            *outLayoutSize = 0;
    }

    return err;
}

// See if there is a channel layout tag for the specified channel layout, and if so, return it.
// Otherwise, return kAudioChannelLayoutTag_UseChannelDescriptions as the tag.
OSStatus getTagForLayout(AudioChannelLayout *inLayout, UInt32 inLayoutSize, AudioChannelLayoutTag *outLayoutTag)
{
    OSStatus	err = noErr;
    UInt32		size = 0;

    if (!inLayout || !outLayoutTag)
    {
        err = paramErr;
        goto bail;
    }

    // The CoreAudio function returns a 'Mono' tag for a single Center channel,
    // which is functionally correct, but can lead to unexpected inconsistencies in channel labeling.
    // We catch this case and don't return a tag at all.
    if ((inLayout->mNumberChannelDescriptions == 1) &&
            (inLayout->mChannelDescriptions[0].mChannelLabel != kAudioChannelLabel_Mono))
    {
        *outLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
    }
    else
    {
        size = sizeof(AudioChannelLayoutTag);
        err = AudioFormatGetProperty(kAudioFormatProperty_TagForChannelLayout,
                                     inLayoutSize, inLayout, &size, outLayoutTag);
        if (err)
            *outLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
    }

bail:
    return err;
}

// Sets the channel layout for the specified track.
// The track layout is copied in, so the caller may free it as soon as this function returns.
OSStatus setTrackLayout(Track track,  UInt32 inLayoutSize, AudioChannelLayout* inLayout)
{
    OSStatus err = noErr;

    if (!inLayout || (inLayoutSize == 0) || (track == nil))
    {
        err = paramErr;
        goto bail;
    }

    // Setting the track layout
    err = QTSetTrackProperty(track,
                             kQTPropertyClass_Audio,
                             kQTAudioPropertyID_ChannelLayout,
                             inLayoutSize,
                             inLayout);
bail:
    return err;
}


// Determine whether or not a given track is likely to be mixing into the Audio Context
// (if not, it is either not an 'ears' track or it is playing directly into SoundManager).
//
// Until QT publishes a property for this, look for tracks that will return an audio channel layout.
Boolean trackMixesToAudioContext(Track track)
{
    UInt32		flags;
    OSStatus	err;

    err = QTGetTrackPropertyInfo(track, kQTPropertyClass_Audio, kQTAudioPropertyID_ChannelLayout,
                                 nil, nil, &flags);
    if (!err && (flags & kComponentPropertyFlagCanGetNow))
        return (true);

    return (false);
}

// Prepare the specified movie for extraction:
//	Open an extraction session.
//	Set the "All Channels Discrete" property if required.
//	Set the ASBD and output layout, if specified.
// Return the audioExtractionSessionRef.
OSStatus prepareMovieForExtraction(Movie movie,
                                   MovieAudioExtractionRef* extractionRefPtr,
                                   Boolean discrete,
                                   AudioStreamBasicDescription asbd,
                                   AudioChannelLayout** layout,
                                   UInt32* layoutSizePtr,
                                   TimeRecord startTime)
{
    OSStatus	err = noErr;

    if (extractionRefPtr == nil)
    {
        err = paramErr;
        goto bail;
    }

    // Movie extraction begin: Open an extraction session
    err = MovieAudioExtractionBegin(movie, 0, extractionRefPtr);
    if (err)
        goto bail;

    // If we need to extract all discrete channels, set that property
    if (discrete)
    {
        err = MovieAudioExtractionSetProperty(*extractionRefPtr,
                                              kQTPropertyClass_MovieAudioExtraction_Movie,
                                              kQTMovieAudioExtractionMoviePropertyID_AllChannelsDiscrete,
                                              sizeof (discrete),
                                              &discrete);
        if (err)
            goto bail;
    }
    // Set the extraction ASBD
    err = MovieAudioExtractionSetProperty(*extractionRefPtr,
                                          kQTPropertyClass_MovieAudioExtraction_Audio,
                                          kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
                                          sizeof (asbd), &asbd);
    if (err)
        goto bail;

    // Set the output layout, if supplied
    if (*layout)
    {
        err = MovieAudioExtractionSetProperty(*extractionRefPtr,
                                              kQTPropertyClass_MovieAudioExtraction_Audio,
                                              kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
                                              *layoutSizePtr, *layout);
        if (err)
            goto bail;
    }

    // Set the extraction start time.  The duration will be determined by how much is pulled.
    err = MovieAudioExtractionSetProperty(*extractionRefPtr,
                                          kQTPropertyClass_MovieAudioExtraction_Movie,
                                          kQTMovieAudioExtractionMoviePropertyID_CurrentTime,
                                          sizeof(TimeRecord), &startTime);
    if (err)
        goto bail;

bail:
    // If error, close the extraction session
    if (err != noErr)
    {
        if (*extractionRefPtr != nil)
            (void) MovieAudioExtractionEnd(*extractionRefPtr);
    }
    return err;
}



// Extract a slice of audio and schedule it for play in the AUScheduledSoundPlayer.
// The ScheduledAudioSlice parameter contains the AudioBufferList to fill with
// extracted audio data (which is always deinterleaved, since we're passing
// it along to an AUGraph).  'playerUnit' specifies an AUScheduledSoundPlayer
// instance to which the filled slice is queued.
OSStatus extractSliceAndScheduleToPlay(MovieAudioExtractionRef extraction,
                                       AudioStreamBasicDescription asbd,
                                       AudioUnit playerUnit,
                                       ScheduledAudioSlice* slice,
                                       SInt64 *ioNumSamples,
                                       Boolean *extractionComplete)
{
    OSStatus		err = noErr;
    UInt32			b;
    UInt32			flags;
    UInt32			numFrames = *ioNumSamples;

    // Make sure the buffer list has all the buffer sizes reset.
    for (b = 0; b < slice->mBufferList->mNumberBuffers; b++) {
        slice->mBufferList->mBuffers[b].mDataByteSize = *ioNumSamples * asbd.mBytesPerPacket;
    }

    // Extract into the slice's bufferlist
    err = MovieAudioExtractionFillBuffer(extraction, &numFrames, slice->mBufferList, &flags);
    *extractionComplete = (flags & kQTMovieAudioExtractionComplete);
    if (!err && (numFrames != 0)) {
        // Fill in the slice frame count
        slice->mNumberFrames = numFrames;
        err = AudioUnitSetProperty(playerUnit,
                                   kAudioUnitProperty_ScheduleAudioSlice,
                                   kAudioUnitScope_Global,
                                   0,
                                   slice,
                                   sizeof(ScheduledAudioSlice));
        if (err)
            goto bail;
    }

bail:
    if (err)
        numFrames = 0;
    *ioNumSamples = numFrames;
    return err;
}

// Extract a slice of PCM audio and write it to an AIFF file.
// Audio extraction proceeds serially from the last position.
// 'ploc' specifies the file offset that this buffer should be written to.
// This could be optimized by supplying a buffer, but for now
// it is simply allocated and released in each call.
OSStatus extractAudioToFile(MovieAudioExtractionRef extraction, AudioFileID outFileID,
                            AudioStreamBasicDescription asbd,
                            SInt64 *ioNumSamples,
                            SInt64 *ploc, Boolean *complete)
{
    OSStatus		err = noErr;
    AudioBufferList	bufList;
    UInt32			bufsize;
    char			*buffer = nil;
    UInt32			flags;
    UInt32			numFrames;

    *complete = false;

    numFrames = *ioNumSamples;

    bufsize = numFrames * asbd.mBytesPerFrame;
    buffer = (char *) malloc(bufsize);
    if (buffer == nil) {
        err = memFullErr;
        goto bail;
    }

    // We always extract interleaved data, since that is all we can write to an AIFF file.
    bufList.mNumberBuffers = 1;
    bufList.mBuffers[0].mNumberChannels = asbd.mChannelsPerFrame;
    bufList.mBuffers[0].mDataByteSize = bufsize;
    bufList.mBuffers[0].mData = buffer;

    // Read the number of requested samples
    err = MovieAudioExtractionFillBuffer(extraction, &numFrames, &bufList, &flags);
    if (err)
        goto bail;

    // Write it to the file
    if (numFrames > 0) {
        err = AudioFileWritePackets(outFileID, false, numFrames * asbd.mBytesPerPacket,
                                    NULL, *ploc, &numFrames, buffer);
        if (err)
            goto bail;
        *ploc += numFrames;
    }

bail:
    if (buffer != nil)
        free (buffer);
    if (err)
        numFrames = 0;
    *ioNumSamples = numFrames;
    *complete = (flags & kQTMovieAudioExtractionComplete);
    return err;
}

#endif



