//
//  TouchBoard.h
//
//
//  Created by Isak Amundsson on 2018-09-28.
//

#ifndef TouchBoard_
#define TouchBoard_

#include "IKAROS.h"


class TouchBoard: public Module
{
public:
    static Module *Create(Parameter * p) { return new TouchBoard(p); }

    TouchBoard(Parameter * p) : Module(p) {}
    virtual ~TouchBoard();

    void Init();
    void PrintValue();
    void Tick();

    Serial *s;
    char    *rcvmsg;
    float *	output;
};

#endif
