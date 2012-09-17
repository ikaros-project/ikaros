//
//	OutputQTAudioVisual.cc  This file is a part of the IKAROS project
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

/*
	File:		CreateMovie.c

	Contains:	QuickTime CreateMovie sample code

	Written by:	Scott Kuechle
				(based heavily on QuickTime sample code in Inside Macintosh: QuickTime)

	Copyright:  © 1998 by Apple Computer, Inc. All rights reserved

	Change History (most recent first)

		<4>		09/01/05	cgp		Ported to Ikaros, and heavily modified.
	  	<3>	 	09/30/98	rtm		tweaked calls to CreateMovieFIle and AddMovieResource to create single-fork movies
		<2>		09/28/98	rtm		changes for Metrowerks compiler
		<1>		06/26/98	srk		first file
*/

#include "OutputQTAudioVisual.h"

#ifdef USE_QUICKTIME_OLD

namespace qt
{
#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
}

using namespace qt;



void CheckError(OSErr error, const char *msg);

void CheckError(OSErr error, const char *msg)
{
    if (error == noErr)
    {
        return;
    }
    if (strlen(msg) > 0)
    {
        printf("%s", msg);
        ExitToShell();
    }
}



//--------


/************************************************************
*                                                           *
*    CONSTANTS                                              *
*                                                           *
*************************************************************/

#define		kVideoTimeScale 	600
#define		kSampleDuration 	60	/* frame duration = 1/10 sec */
#define		kNumVideoFrames 	70

//#define		kVideoTimeScale 	2997
//#define		kSampleDuration 	100	/* 29.97 fps */
//#define		kNumVideoFrames 	210

#define		kPixelDepth 		16	/* use 16-bit depth */
#define		kNoOffset 			0
#define		kMgrChoose			0
#define		kSyncSample 		0
#define		kAddOneVideoSample	1
#define		kTrackStart			0
#define		kMediaStart			0

class VisualTrack
{
public:
    // scale gives the number of time units that elapse per second
    // sampleDuration gives the number of time units that elapse per visual frame
    VisualTrack(Movie theMovie, int height, int width, TimeScale scale, int sampleDuration);

    // the visualFrame is row-major, and has height by width float values in it.
    void AddFrame(float *visualFrame, float upper_bound_pixel_visual_input);

    void Close(); // close the visual track

private:
    Track m_Track;
    Media m_Media;
    Rect m_size;
    GWorldPtr m_GWorld;
    Handle m_compressedData;
    Ptr m_compressedDataPtr;
    ImageDescriptionHandle m_imageDesc;
    CGrafPtr m_oldPort;
    GDHandle m_oldGDeviceH;
    PixMapHandle m_pmh;
    PixMapPtr m_pmp;
    int m_numBytesPerRow;
    int m_sampleDuration;
};


VisualTrack::VisualTrack(Movie theMovie, int height, int width, TimeScale scale, int sampleDuration)
{
    m_sampleDuration = sampleDuration;
    m_GWorld = nil;
    m_compressedData = nil;
    m_imageDesc = nil;

    m_size.right = width;
    m_size.bottom = height;
    m_size.top = 0;
    m_size.left = 0;

    OSErr err = noErr;

    // 1. Create the track
    m_Track = NewMovieTrack (theMovie, 					 /* movie specifier */
                             FixRatio(m_size.right,1),  /* width */
                             FixRatio(m_size.bottom,1), /* height */
                             kNoVolume); 					 /* trackVolume */
    CheckError( GetMoviesError(), "NewMovieTrack error" );

    // 2. Create the media for the track
    m_Media = NewTrackMedia (m_Track,			/* track identifier */
                             VideoMediaType,	/* type of media */
                             scale, 	/* time coordinate system */
                             nil,				/* data reference - use the file that is associated with the movie  */
                             0);				/* data reference type */
    CheckError( GetMoviesError(), "NewTrackMedia error" );

    // 3. Establish a media-editing session
    err = BeginMediaEdits (m_Media);
    CheckError( err, "BeginMediaEdits error" );

    // Setup for adding samples

    // Create a graphics world
    err = NewGWorld (&m_GWorld,		/* pointer to created gworld */
                     kPixelDepth,		/* pixel depth */
                     &m_size, 		/* bounds */
                     nil, 				/* color table */
                     nil,				/* handle to GDevice */
                     (GWorldFlags)0);	/* flags */
    CheckError (err, "NewGWorld error");

    // Lock the pixels
    LockPixels (GetPortPixMap(m_GWorld));

    // Determine the maximum size the image will be after compression.
    // Specify the compression characteristics, along with the image.
    long maxCompressedSize;
    err = GetMaxCompressionSize(GetPortPixMap(m_GWorld),		/* Handle to the source image */
                                &m_size, 					/* bounds */
                                kMgrChoose, 					/* let ICM choose depth */
                                codecNormalQuality,				/* desired image quality */
                                kAnimationCodecType,			/* compressor type */
                                (CompressorComponent)anyCodec,  /* compressor identifier */
                                &maxCompressedSize);		    /* returned size */
    CheckError (err, "GetMaxCompressionSize error" );

    // Create a new handle of the right size for our compressed image data
    m_compressedData = NewHandle(maxCompressedSize);
    CheckError( MemError(), "NewHandle error" );

    MoveHHi( m_compressedData );
    HLock( m_compressedData );
    m_compressedDataPtr = *m_compressedData;

    // Create a handle for the Image Description Structure
    m_imageDesc = (ImageDescriptionHandle)NewHandle(4);
    CheckError( MemError(), "NewHandle error" );

    // Change the current graphics port to the GWorld
    GetGWorld(&m_oldPort, &m_oldGDeviceH);
    SetGWorld(m_GWorld, nil);

    m_pmh = GetPortPixMap(m_GWorld);
    m_pmp = *m_pmh;

    // pixel size is 0
    printf("pixel size= %d\n", m_pmp->pixelSize);
    // top and left will be 0
    printf("top= %d, left= %d, right= %d, bottom= %d\n", m_pmp->bounds.top, m_pmp->bounds.left, m_pmp->bounds.right, m_pmp->bounds.bottom);
    m_numBytesPerRow = ((short) ~(0xC000) & m_pmp->rowBytes);
    printf("m_numBytesPerRow= %d\n", m_numBytesPerRow);
}


void VisualTrack::Close()
{
    OSErr err = noErr;

    SetGWorld (m_oldPort, m_oldGDeviceH);

    // Dealocate our previously alocated handles and GWorld
    if (m_imageDesc)
    {
        DisposeHandle ((Handle)m_imageDesc);
    }

    if (m_compressedData)
    {
        DisposeHandle (m_compressedData);
    }

    if (m_GWorld)
    {
        DisposeGWorld (m_GWorld);
    }

    // 3b. End media-editing session
    err = EndMediaEdits (m_Media);
    CheckError( err, "EndMediaEdits error" );

    // 4. Insert a reference to a media segment into the track
    err = InsertMediaIntoTrack (m_Track,		/* track specifier */
                                kTrackStart,	/* track start time */
                                kMediaStart, 	/* media start time */
                                GetMediaDuration(m_Media), /* media duration */
                                fixed1);		/* media rate ((Fixed) 0x00010000L) */
    CheckError( err, "InsertMediaIntoTrack error" );
}



void VisualTrack::AddFrame(float *visualFrame, float upper_bound_pixel_visual_input)
{
    OSErr err = noErr;
    Ptr data = m_pmp->baseAddr;
    float pixel;

    int row, col;
    for (row=0; row < m_pmp->bounds.bottom; row++)
    {
        for (col=0; col < m_pmp->bounds.right*(m_pmp->pixelSize/8); col += m_pmp->pixelSize/8)
        {
            pixel = *visualFrame;

            if (pixel > upper_bound_pixel_visual_input)
            {
                pixel = upper_bound_pixel_visual_input;
            }
            else if (pixel < 0)
            {
                pixel = 0.0;
            }

            // Assuming 16 bit pixel values represented as unsigned short values
            *((unsigned short *) (data + col)) = (unsigned short) (0xFFFF * (pixel/upper_bound_pixel_visual_input));

            visualFrame++;
        }
        //printf("\n");

        data += m_numBytesPerRow;
    }

    m_pmp->pixelFormat = k16GrayPixelFormat;

    // Use the ICM to compress the image
    err = CompressImage(m_pmh, /* source image to compress */
                        &m_size, 			  /* bounds */
                        codecNormalQuality,		  /* desired image quality */
                        kAnimationCodecType,	  /* compressor identifier */
                        m_imageDesc, 				  /* handle to Image Description Structure; will be resized by call */
                        m_compressedDataPtr);		  /* pointer to a location to recieve the compressed image data */
    CheckError( err, "CompressImage error" );

    // Add sample data and a description to a media
    err = AddMediaSample(m_Media,				 /* media specifier */
                         m_compressedData,		 /* handle to sample data - dataIn */
                         kNoOffset,				 /* specifies offset into data reffered to by dataIn handle */
                         (**m_imageDesc).dataSize, /* number of bytes of sample data to be added */
                         m_sampleDuration,		 /* frame duration */
                         (SampleDescriptionHandle)m_imageDesc,	/* sample description handle */
                         kAddOneVideoSample,	/* number of samples */
                         kSyncSample,			/* control flag indicating self-contained samples */
                         nil);					/* returns a time value where sample was insterted */
    CheckError( err, "AddMediaSample error" );
}



/************************************************************
*                                                           *
*    CONSTANTS                                              *
*                                                           *
*************************************************************/

#define	kOurSoundResourceID 128

#define	kSoundSampleDuration 1
//#define	kSyncSample 0
//#define	kTrackStart	0
//#define	kMediaStart	0
/*
for the following constants, please consult the Macintosh
Audio Compression and Expansion Toolkit
*/
#define kMACEBeginningNumberOfBytes 6
#define kMACE31MonoPacketSize 2
#define kMACE31StereoPacketSize 4
#define kMACE61MonoPacketSize 1
#define kMACE61StereoPacketSize 2

class SoundTrack
{
public:
    SoundTrack(Movie theMovie, float sampleRate, int numAudioChannels);

    // numAudioSamples indicate the number of audio frames in the audioSamples vector
    // The number of float samples is numAudioFrames * numAudioChannels
    void AddSamples(float *audioSamples, int numAudioFrames);
    void Close();

private:
    Track m_Track;
    Media m_Media;
    SoundDescriptionV2Handle m_sndDesc;
    SoundDescriptionV2Ptr m_sndDescPtr;
    int m_numAudioChannels;
};



/************************************************************
*                                                           *
*    TYPE DEFINITIONS                                       *
*                                                           *
*************************************************************/

typedef SndCommand *SndCmdPtr;

#if PRAGMA_STRUCT_ALIGN
#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
#pragma pack(2)
#endif

/* 'snd ' resource format 1 - see the Sound Manager chapter of Inside Macintosh: Sound
	for the details */
typedef struct
{
    short format;
    short numSynths;
}
Snd1Header, *Snd1HdrPtr, **Snd1HdrHndl;

/* 'snd ' resource format 2 - see the Sound Manager chapter of Inside Macintosh: Sound
	for the details */

typedef struct
{
    short format;
    short refCount;
}
Snd2Header, *Snd2HdrPtr, **Snd2HdrHndl;


#if PRAGMA_STRUCT_ALIGN
#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
#pragma pack()
#endif


SoundTrack::SoundTrack(Movie theMovie, float sampleRate, int numAudioChannels)
{
    OSErr err = noErr;
    AudioStreamBasicDescription asbd;

    m_numAudioChannels = numAudioChannels;

    asbd.mSampleRate = sampleRate;

    // I got the some of the following values empirically by looking at another asbd
    asbd.mReserved = 0;
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsBigEndian;

    asbd.mBytesPerPacket = 4;
    asbd.mFramesPerPacket = 1;
    asbd.mBytesPerFrame = 4;
    asbd.mBitsPerChannel = 32;

    // Question: What is the format for multiple channel audio in the audio buffer passed into AddMediaSample2?
    asbd.mChannelsPerFrame = m_numAudioChannels;

    OSStatus err2 = QTSoundDescriptionCreate (
                        &asbd, // AudioStreamBasicDescription
                        NULL, // AudioChannelLayout
                        0, // ByteCount
                        NULL,
                        0,
                        kQTSoundDescriptionKind_Movie_Version2,
                        (SoundDescriptionHandle *) &m_sndDesc);

    if (err2 != noErr)
    {
        printf("SoundTrack::SoundTrack: Error calling QTSoundDescriptionCreate\n");
        return;
    }

    m_sndDescPtr = *m_sndDesc;

    // 1. Create the track
    m_Track = NewMovieTrack (theMovie,		/* movie specifier */
                             0,			/* width */
                             0,			/* height */
                             kFullVolume);	/* track volume */
    CheckError (GetMoviesError(), "NewMovieTrack error" );

    // 2. Create the media for the track
    m_Media = NewTrackMedia (m_Track,				/* track specifier */
                             SoundMediaType,		/* type of media */
                             (TimeScale)m_sndDescPtr->audioSampleRate,	/* time coordinate system */
                             nil,					/* data reference */
                             0);					/* data reference type */
    CheckError (GetMoviesError(), "NewTrackMedia error" );

    // 3. Establish a media-editing session
    err = BeginMediaEdits(m_Media);
    CheckError( err, "BeginMediaEdits error" );

    printf("End of constructor\n");
    fflush(stdout);
}

void SoundTrack::AddSamples(float *audioSamples, int numAudioFrames)
{
    OSErr err = noErr;

    // constBitsPerChannel is in units of bits, so divide by 8 bits per byte
    int sndDataSize = numAudioFrames * m_sndDescPtr->constBitsPerChannel/8 * m_numAudioChannels;

    err = AddMediaSample2(  m_Media,						// the Media
                            (const UInt8 *) audioSamples,   // const UInt8 * dataIn
                            sndDataSize,                    // ByteCount size
                            1,                              // TimeValue64 decodeDurationPerSample
                            0,                              // TimeValue64 displayOffset
                            (SampleDescriptionHandle)m_sndDesc, // SampleDescriptionHandle sampleDescriptionH
                            numAudioFrames,					// ItemCount numberOfSamples-- CGP-- I take number of samples here to be the same as the number of audio frames
                            0,                              // MediaSampleFlags sampleFlags
                            NULL);                          // TimeValue64 * sampleDecodeTimeOut
    if ( err )
    {
        fprintf( stderr, "AddMediaSample2(soundMedia) failed (%d)\n", err );
    }

    //printf("End of AddSamples\n"); fflush(stdout);
}

void SoundTrack::Close()
{
    OSErr err = noErr;

    // 3b. End media-editing session
    err = EndMediaEdits(m_Media);
    CheckError( err, "EndMediaEdits error" );

    // 4. Inserts a reference to a media segment into the track
    err = InsertMediaIntoTrack (m_Track,		/* track specifier */
                                kTrackStart,	/* track start time */
                                kMediaStart,	/* media start time */
                                GetMediaDuration (m_Media),	 /* media duration */
                                fixed1);		/* media rate ((Fixed) 0x00010000L) */
    CheckError( err, "InsertMediaIntoTrack error" );

    if (m_sndDesc != nil)
    {
        DisposeHandle( (Handle)m_sndDesc);
    }

    printf("SoundTrack::Close: End of Close\n");
    fflush(stdout);
}



OutputQTAudioVisual::OutputQTAudioVisual(Parameter * p):
        Module(p)
{
    m_filename = GetValue("filename");
    if (m_filename == NULL)
    {
        Notify(msg_fatal_error, "ERROR: No output file parameter supplied\n");
        return;
    }

    m_visual_time_scale = GetIntValue("visual_time_scale", 2997);
    m_visual_sample_duration = GetIntValue("visual_sample_duration", 100);

    m_audio_sample_rate = GetIntValue("audio_sample_rate", 44100);

    m_upper_bound_pixel_visual_input = GetFloatValue("upper_bound_pixel_visual_input", 1.0);

    printf("OutputQTAudioVisual::OutputQTAudioVisual: visual_time_scale= %d\n", m_visual_time_scale);
    printf("OutputQTAudioVisual::OutputQTAudioVisual: visual_sample_duration= %d\n", m_visual_sample_duration);
    printf("OutputQTAudioVisual::OutputQTAudioVisual: audio_sample_rate= %d\n", m_audio_sample_rate);

    AddInput("VISUAL");

    AddInput("AUDIO_CHANNELS");
    AddInput("NUMBER_AUDIO_SAMPLES");

    OSErr result = noErr;

    // Initialize the QuickTime Movie Toolbox
    result = EnterMovies();
    if (result != noErr)
        return;
}



void
OutputQTAudioVisual::Init()
{
    m_visualWidth = GetInputSizeX("VISUAL");
    m_visualHeight = GetInputSizeY("VISUAL");
    m_visual = GetInputArray("VISUAL");

    m_numAudioSamples = GetInputArray("NUMBER_AUDIO_SAMPLES");
    m_numAudioChannels = GetInputSizeX("AUDIO_CHANNELS");
    m_audio = GetInputArray("AUDIO_CHANNELS");

    // Need the visual height and width before opening the movie.
    OpenMovie();
}



OutputQTAudioVisual::~OutputQTAudioVisual()
{
    CloseMovie();
    printf("OutputQTAudioVisual::~OutputQTAudioVisual: Destructor called!!\n");
}



Module *
OutputQTAudioVisual::Create(Parameter * p)
{
    return new OutputQTAudioVisual(p);
}

//static int count = 0;

void
OutputQTAudioVisual::Tick()
{
    /*
    count++;
    if (count == 30)
    	CloseMovie();
    else if (count < 30) {
    */

    m_visualTrack->AddFrame(m_visual, m_upper_bound_pixel_visual_input);
    m_soundTrack->AddSamples(m_audio, (int) *m_numAudioSamples);
    //}
}


// PathToFSSpec is from http://www.cocoadev.com/index.pl?FSMakeFSSpec

OSStatus PathToFSSpec (const char *inPath, FSSpec *outSpec);

OSStatus PathToFSSpec (const char *inPath, FSSpec *outSpec)
{
    OSStatus err = noErr;
    FSRef ref;
    Boolean isDirectory;
    FSCatalogInfo info;
    CFStringRef pathString = NULL;
    CFURLRef pathURL = NULL;
    CFURLRef parentURL = NULL;
    CFStringRef nameString = NULL;

    // First, try to create an FSRef for the full path
    if (err == noErr)
    {
        err = FSPathMakeRef ((UInt8 *) inPath, &ref, &isDirectory);
    }

    if (err == noErr)
    {
        // It's a directory or a file that exists; convert directly into an FSSpec:
        err = FSGetCatalogInfo (&ref, kFSCatInfoNone, NULL, NULL, outSpec, NULL);
    }
    else
    {
        // The harder case.  The file doesn't exist.
        err = noErr;

        // Get a CFString for the path
        if (err == noErr)
        {
            pathString = CFStringCreateWithCString (CFAllocatorGetDefault (), inPath, CFStringGetSystemEncoding ());
            if (pathString == NULL)
            {
                err = memFullErr;
            }
        }

        // Get a CFURL for the path
        if (err == noErr)
        {
            pathURL = CFURLCreateWithFileSystemPath (CFAllocatorGetDefault (),
                      pathString, kCFURLPOSIXPathStyle,
                      false /* Not a directory */);
            if (pathURL == NULL)
            {
                err = memFullErr;
            }
        }

        // Get a CFURL for the parent
        if (err == noErr)
        {
            parentURL = CFURLCreateCopyDeletingLastPathComponent (CFAllocatorGetDefault (), pathURL);
            if (parentURL == NULL)
            {
                err = memFullErr;
            }
        }

        // Build an FSRef for the parent directory, which must be valid to make an FSSpec
        if (err == noErr)
        {
            Boolean converted = CFURLGetFSRef (parentURL, &ref);
            if (!converted)
            {
                err = fnfErr;
            }
        }

        // Get the node ID of the parent directory
        if (err == noErr)
        {
            err = FSGetCatalogInfo(&ref, kFSCatInfoNodeFlags|kFSCatInfoNodeID, &info, NULL, outSpec, NULL);
        }

        // Get a CFString for the file name
        if (err == noErr)
        {
            nameString = CFURLCopyLastPathComponent (pathURL);
            if (nameString == NULL)
            {
                err = memFullErr;
            }
        }

        // Copy the string into the FSSpec
        if (err == noErr)
        {
            Boolean converted = CFStringGetPascalString (nameString, outSpec->name, sizeof (outSpec->name),
                                CFStringGetSystemEncoding ());
            if (!converted)
            {
                err = fnfErr;
            }

        }

        // Set the node ID in the FSSpec
        if (err == noErr)
        {
            outSpec->parID = info.nodeID;
        }
    }

    // Free allocated memory
    if (pathURL != NULL)
    {
        CFRelease (pathURL);
    }
    if (pathString != NULL)
    {
        CFRelease (pathString);
    }
    if (parentURL != NULL)
    {
        CFRelease (parentURL);
    }
    if (nameString != NULL)
    {
        CFRelease (nameString);
    }

    return err;
}


/*
Sample Player's creator type since it is the movie player
of choice. You can use your own creator type, of course.
*/
const UInt32 kMyCreatorType = 1; // ('TVOD');

void OutputQTAudioVisual::OpenMovie()
{
    m_Movie = nil;
    m_resRefNum = 0;
    FSSpec mySpec;
    OSErr err = noErr;

    err = PathToFSSpec (m_filename, &mySpec);
    if ((err != fnfErr) && (err != noErr))
    {
        printf("Error calling PathToFSSpec for file= %s (err= %d; and not %d or %d)\n", m_filename, err, fnfErr, noErr);
        return;
    }

    // Create and open the movie file, this call creates an empty movie which
    // references the file, and opens the movie file with write permission.
    err = CreateMovieFile(&mySpec,							/* FSSpec specifier */
                          kMyCreatorType,					/* file creator type */
                          smCurrentScript,					/* movie file creation flags */
                          createMovieFileDeleteCurFile |
                          createMovieFileDontCreateResFile |
                          newMovieActive,
                          &m_resRefNum,                     /* file ref num */
                          (qt::MovieType***)(&m_Movie));    // TODO: Check that this works

    if ((err != fnfErr) && (err != noErr))
    {
        printf("Error calling CreateMovieFile for file= %s (err= %d; and not %d or %d)\n", m_filename, err, fnfErr, noErr);
        return;
    }

    // Call our functions to create the video track and the sound track.
    m_visualTrack = new VisualTrack((qt::Movie)(m_Movie), m_visualHeight, m_visualWidth, m_visual_time_scale, m_visual_sample_duration);

    // For now, this really only can use one channel-- we're assuming the audio output module is only supplying a single channel
    // when we are using this Quicktime output module.
    m_soundTrack = new SoundTrack((qt::Movie)(m_Movie), (float) m_audio_sample_rate, m_numAudioChannels);
}


void OutputQTAudioVisual::CloseMovie()
{
    OSErr err = noErr;
    short resId = movieInDataForkResID;

    // Add the movie resource to the movie file. We use movieInDataForkResID for the resID.
    // This will add the movie resource to the file's data fork for a single-fork movie file
    // instead of adding the resource to the file's resource fork.
    err = AddMovieResource((qt::Movie)(m_Movie),			/* movie specification */
                           m_resRefNum,			/* file ref num */
                           &resId,				/* movie resource id */
                           (const unsigned char *) m_filename);			/* name of the movie resource */
    CheckError(err, "AddMovieResource error");

    m_visualTrack->Close();
    delete m_visualTrack;

    m_soundTrack->Close();
    delete m_soundTrack;

    if (m_resRefNum)
    {
        // Close our open movie file
        CloseMovieFile (m_resRefNum);
    }

    printf("OutputQTAudioVisual::CloseMovie: Called\n");
}
#endif

