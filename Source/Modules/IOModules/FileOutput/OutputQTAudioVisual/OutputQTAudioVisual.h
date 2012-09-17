//
//	OutputQTAudioVisual.h		This file is a part of the IKAROS project
//							A module for writing Quicktime files with both audio and visual tracks.
//
//    Copyright (C) 2001-2002  Christian Balkenius
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
//

//	Changes 2007-01-08 by Christian Balkenius:
//
//		namespaces were added to isolate the QT code from Ikaros XML
//		everything was put in the same file to aboid collisions
//		it is ugly, but it works
//
//      CB; 24 July 2009; moved all QT code to data object

#ifndef OUTPUTQTAUDIOVISUAL
#define OUTPUTQTAUDIOVISUAL

#include "IKAROS.h"

#ifdef USE_QUICKTIME_OLD

class VisualTrack;
class SoundTrack;

class OutputQTAudioVisual: public Module
{
public:
    OutputQTAudioVisual(Parameter * p);		// Parameter list should contain 'column' with 'inputs'
    virtual ~OutputQTAudioVisual();

    static Module * Create(Parameter * p);

    void Init();
    void Tick();							// Write the data at the inputs to the file

private:
    void OpenMovie();
    void CloseMovie();

private:
    void ** m_Movie;      // hides type ot avoid namespace conflicts, is really qt::Movie

    VisualTrack *m_visualTrack;
    SoundTrack *m_soundTrack;
    short m_resRefNum;
    const char *m_filename;

    int m_visual_time_scale; // the number of time units that elapse per second in the visual track
    int m_visual_sample_duration; // the number of time units that elapse per visual frame in the visual track

    int m_audio_sample_rate; // the number of audio samples per second

    float m_upper_bound_pixel_visual_input; // the maximum value for each pixel used as inputs in the visual track; assume 0 lower bound

    int m_visualWidth;
    int m_visualHeight;
    float *m_visual;

    float *m_numAudioSamples;
    int m_numAudioChannels;
    float *m_audio;
};

#endif

#ifndef USE_QUICKTIME_OLD
class OutputQTAudioVisual
{
public:
    static Module * Create(Parameter * p)
    {
        return NULL;
    }
};
#endif



#endif
