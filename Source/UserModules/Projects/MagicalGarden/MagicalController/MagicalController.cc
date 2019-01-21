//
//      MagicalController.cc		This file is a part of the IKAROS project
//				
//
//      Copyright (C) 2017 Christian Balkenius
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//


#include "MagicalController.h"

using namespace ikaros;

void
MagicalController::Init()
{
    behavior.insert( { "Say", 0 });
    behavior.insert( { "Wave", 1 });
    behavior.insert( { "JumpPump", 2 });
    behavior.insert( { "Happy", 3 });
    behavior.insert( { "Neutral", 4 });
    behavior.insert( { "Focused", 5 });
    behavior.insert( { "FistPump", 6 });

    sound.insert( { "fewer", 0} );
    sound.insert( { "goodTeacher",  1} );
    sound.insert( { "hasToBeThis", 2} );
    sound.insert( { "higher", 3} );
    sound.insert( { "hmm", 4} );
    sound.insert( { "instructionGreen", 5} );
    sound.insert( { "instructionRed", 6} );
    sound.insert( { "isItThisOne", 7} );
    sound.insert( { "isThisRight", 8} );
    sound.insert( { "letsGo", 9} );
    sound.insert( { "lower", 10} );
    sound.insert( { "more", 11} );
    sound.insert( { "myTurn1", 12} );
    sound.insert( { "myTurn2", 13} );
    sound.insert( { "ok1", 14} );
    sound.insert( { "ok2", 15} );
    sound.insert( { "tryAgain", 16} );
    sound.insert( { "wasCorrect", 17} );
    sound.insert( { "whatISaid", 18} );
    sound.insert( { "whatIThought", 19} );
    sound.insert( { "willYouHelpMe", 20} );
    sound.insert( { "wrong1", 21} );
    sound.insert( { "wrong2", 22} );
    sound.insert( { "wrongShow", 23} );
    sound.insert( { "ballongamepandaIntro", 24} );
    sound.insert( { "beeflightpandaIntro2", 25} );
    sound.insert( { "beeflightpandaIntro3",  26} );
    sound.insert( { "birdheropandaIntro", 27} );
    sound.insert( { "lizardpandaIntro1", 28} );
    sound.insert( { "lizardpandaIntro2", 29} );
    sound.insert( { "vehiclepandaIntro1", 30} );
    sound.insert( { "vehiclepandaIntro2", 31} );

    sound_len.insert( { "fewer", 1.55} );
    sound_len.insert( { "goodTeacher",  2.15} );
    sound_len.insert( { "hasToBeThis", 1.75} );
    sound_len.insert( { "higher", 1.55} );
    sound_len.insert( { "hmm", 1.25} );
    sound_len.insert( { "instructionGreen", 3.15} );
    sound_len.insert( { "instructionRed", 3.25} );
    sound_len.insert( { "isItThisOne", 1.15} );
    sound_len.insert( { "isThisRight", 1.45} );
    sound_len.insert( { "letsGo", 0.85} );
    sound_len.insert( { "lower", 2.05} );
    sound_len.insert( { "more", 1.15} );
    sound_len.insert( { "myTurn1", 2.35} );
    sound_len.insert( { "myTurn2", 1.35} );
    sound_len.insert( { "ok1", 0.55} );
    sound_len.insert( { "ok2", 0.55} );
    sound_len.insert( { "tryAgain", 2.35} );
    sound_len.insert( { "wasCorrect", 1.75} );
    sound_len.insert( { "whatISaid", 1.35} );
    sound_len.insert( { "whatIThought", 1.65} );
    sound_len.insert( { "willYouHelpMe", 0.85} );
    sound_len.insert( { "wrong1", 1.55} );
    sound_len.insert( { "wrong2", 1.45} );
    sound_len.insert( { "wrongShow", 3.25} );
    sound_len.insert( { "ballongamepandaIntro", 6.35} );
    sound_len.insert( { "beeflightpandaIntro2", 2.85} );
    sound_len.insert( { "beeflightpandaIntro3",  3.45} );
    sound_len.insert( { "birdheropandaIntro", 6.75} );
    sound_len.insert( { "lizardpandaIntro1", 4.55} );
    sound_len.insert( { "lizardpandaIntro2", 4.05} );
    sound_len.insert( { "vehiclepandaIntro1", 3.65} );
    sound_len.insert( { "vehiclepandaIntro2", 4.85} );




    socket = new ServerSocket(GetIntValue("port"));
    animation_trig = GetOutputArray("ANIMATION_TRIG");
    sound_trig = GetOutputArray("SOUND_TRIG");
    sound_active = GetOutputArray("SOUND_ACTIVE");

    target = GetOutputArray("TARGET");
    
    no_of_animations = GetIntValue("no_of_animations");
    no_of_sounds = GetIntValue("no_of_sounds");
    
    sound_duration = 0;
}

/*

fewer: 1.55
goodTeacher: 2.15
hasToBeThis: 1.75
higher: 1.55
hmm: 1.25
instructionGreen: 3.15
instructionRed: 3.25
isItThisOne: 1.15
isThisRight: 1.45
letsGo: 0.85
lower: 2.05
more: 1.15
myTurn1: 2.35
myTurn2: 1.35
ok1: 0.55
ok2: 0.55
tryAgain: 2.35
wasCorrect: 1.75
whatISaid: 1.35
whatIThought: 1.65
willYouHelpMe: 0.85
wrong1: 1.55
wrong2: 1.45
wrongShow: 3.25
ballongamepandaIntro: 6.35
beeflightpandaIntro2: 2.85
beeflightpandaIntro3: 3.45
birdheropandaIntro: 6.75
lizardpandaIntro1: 4.55
lizardpandaIntro2: 4.05
vehiclepandaIntro1: 3.65
vehiclepandaIntro2: 4.85

*/


void
MagicalController::Tick() // absolutely no error handling!!!
{
    if(sound_timer.GetTime() > sound_duration)
        sound_active[0] = 0;
        

    reset_array(animation_trig, no_of_animations);
    reset_array(sound_trig, no_of_sounds);

    if (socket->GetRequest(false))
    {
        if (equal_strings(socket->header.Get("Method"), "GET"))
        {
            char * uri = create_string(socket->header.Get("URI"));
            printf("MagicalController received: %s\n", uri);
            
            switch(uri[1])
            {
                case 'a': // trigger animation
                    printf("=> %d\n", behavior[&uri[11]]);
                    animation_trig[behavior[&uri[11]]] = 1;
                    break;
                    
                case 's': // trigger sound
                    if(sound.find(&uri[7]) != sound.end())
                        printf("=> %d\n", sound[&uri[7]]);
                    else
                        printf("Not found: %s\n", &uri[7]);
                    sound_trig[sound[&uri[7]]] = 1;
                    sound_active[0] = 1;
                    sound_duration = 1000*sound_len[&uri[7]];
                    sound_timer.Restart();
                    break;
                    
                case 't': // set target
                   target[0] = string_to_float(&uri[8]);
                    target[1] = string_to_float(strstr(uri, ",")+1); // Don't you love string processing in C?
                    break;
            
            }
            
            destroy_string(uri);
        }

        // Reply with OK
        
		Dictionary header;
		header.Set("Content-Type", "text/plain");
		socket->SendHTTPHeader(&header);
        socket->Send("OK\n");
        socket->Close();
    }
}



static InitClass init("MagicalController", &MagicalController::Create, "Source/UserModules/MagicalGarden/MagicalController/");


