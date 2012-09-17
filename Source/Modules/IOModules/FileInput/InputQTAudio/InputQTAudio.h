//
//	InputQTAudio.h		This file is a part of the IKAROS project
//						A module for reading audio from a QuickTime movie
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

#ifndef INPUTQTAUDIO
#define INPUTQTAUDIO

#include "IKAROS.h"

#ifdef USE_QUICKTIME_OLD

class InputQTAudioData;

class InputQTAudio: public Module {
public:
    InputQTAudioData    *   data;

    float *                         channels; // audio channels for output from this module
    float *                         numberSamples; // number of samples for output from this module
    int                             m_numChannels; // the number of audio channels
    int                             m_numChannelsRequested; // the number of channels requested in the XML text (should be <= number of actual channels)

    const char *        filename;
    unsigned long       GetNumSamplesInNextFrame();    // Return the number of audio samples corresponding to the next visual frame

    void				OpenMovie(unsigned char * filename);
    void                InitAudio();

    InputQTAudio(Parameter * p);
    virtual			~InputQTAudio();

    static Module *		Create(Parameter * p);

    void				Init();
    void				Tick();
};

#endif

#ifndef USE_QUICKTIME_OLD
class InputQTAudio {
public:
    static Module * Create(Parameter * p) { return NULL; }
};
#endif


#endif
