/*
 *  MTAudioBufferListUtils.h
 *  MTCoreAudio
 *
 *  Created by Michael Thornburgh on Fri Apr 02 2004.
 *  Copyright (c) 2004 Michael Thornburgh. All rights reserved.
 *
 */

#include "IKAROS.h"

#ifdef USE_QUICKTIME_OLD

namespace qt {
#include <AudioToolbox/AudioToolbox.h>
}

using namespace qt;

AudioBufferList * MTAudioBufferListNew ( unsigned channels, unsigned frames, Boolean interleaved );
void MTAudioBufferListDispose ( AudioBufferList * aList );
unsigned MTAudioBufferListCopy ( const AudioBufferList * src, unsigned srcOffset, AudioBufferList * dst, unsigned dstOffset, unsigned count );
unsigned MTAudioBufferListClear ( AudioBufferList * aList, unsigned offset, unsigned count );
unsigned MTAudioBufferListFrameCount ( const AudioBufferList * buf );
unsigned MTAudioBufferListChannelCount ( const AudioBufferList * buf );

#endif


