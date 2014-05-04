//
//	SoundOutput.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2013-2014 Christian Balkenius
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
//    See http://www.ikaros-project.org/ for more information.
//


#include "SoundOutput.h"

#include <unistd.h>

using namespace ikaros;



static int
count_commas(const char * s)
{
    int c = 0;
    size_t l = strlen(s);
    for(int i=0; i<l; i++)
        if(s[i] == ',')
            c++;
    return c;
}



static const char **
split(const char * s, int & c)
{
    int i = 0;
    c = count_commas(s) + 1;
    const char ** t = new const char * [c];
    char * token;
    char * string = create_string(s);

    while ((token = strsep(&string, ",")) != NULL)
        t[i++] = token;

    return t;
}



static void
play(const char * sound)
{
    char command[] = "afplay";

    char * argv[3] = { NULL, NULL, NULL };

    argv[0] = command;
    argv[1] = (char *)sound;

    if(!vfork())
        execvp(command, argv);
}



void
SoundOutput::Init()
{
    input = GetInputArray("INPUT");
    size = GetInputSize("INPUT");
    last_input = create_array(size);
    command = GetValue("command");
    sound = split(GetValue("sounds"), sound_count);
}

// osascript -e 'say "Dum dum dee dum dum dum dum dee Dum dum dee dum dum dum dum dee dum dee dum dum dum de dum dum dum dee dum dee dum dum dee dummmmmmmmmmmmmmmmm" using "Pipe Organ"
// osascript -e 'say "oh This is a silly song silly song silly song this is the silliest song ive ever ever heard So why keep you listening listening listening while you are supposed to work to work to work to work its because i hate my job hate my job hate my job its because i hate my job more than anything else No its because youve no life youve no life youve no life and you better go get one after forwarding this crap" using "cellos"'


/*
static void say(const char * msg)
{
    int child_id = fork();
    if(!child_id)
    {
        char *s = create_formatted_string("say \"%s\" &", msg);
        system(s);
        destroy_string(s);
        exit(0);
    }
}

*/



void
SoundOutput::Tick()
{
    for(int i=0; i<size; i++)
        if(input[i] == 1 && last_input[i] == 0 && i < sound_count)    // Trig sound # i
            play(sound[i]);

    copy_array(last_input, input, size);
}



static InitClass init("SoundOutput", &SoundOutput::Create, "Source/Modules/IOModules/SoundOutput/");


