//
//  TouchBoard.cpp
//
//
//  Created by Isak Amundsson on 2018-09-28.
//  Modified by Christian Balkenius 2020-09-17
//

#include "TouchBoard.h"

using namespace ikaros;

TouchBoard::~TouchBoard()
{
    if(s)
        s->Close();
}


void
TouchBoard::Init()
{
    s = new Serial(GetValue("port"), 115200);
    s->Flush();
    rcvmsg = new char [1000];
    io(output, "OUTPUT");
    io(touch, "TOUCH");
}


void 
TouchBoard::Tick()
{
    int d[13];

    if(!s)
        return; // Not connected to board

    int count = s->ReceiveUntil(rcvmsg, '\n');
    rcvmsg[count] = 0;

    //printf("%s", rcvmsg);

    if(starts_with(rcvmsg, "DIFF:"))
    {
        sscanf(rcvmsg, "DIFF: %d %d %d %d %d %d %d %d %d %d %d %d", &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7], &d[8], &d[9], &d[10], &d[11]);
        for(int i=0; i<12; i++)
            output[i] = clip(float(d[i])/1024.0, 0, 1);
    }

    if(starts_with(rcvmsg, "TOUCH:"))
    {
        sscanf(rcvmsg, "TOUCH: %d %d %d %d %d %d %d %d %d %d %d %d", &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7], &d[8], &d[9], &d[10], &d[11]);
        for(int i=0; i<12; i++)
            touch[i] = float(d[i]);
    }
}


static InitClass init("TouchBoard", &TouchBoard::Create, "Source/Modules/IOModules/TouchBoard/");
