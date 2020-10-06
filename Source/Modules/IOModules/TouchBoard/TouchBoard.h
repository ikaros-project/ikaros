//
//  TouchBoard.h
//
//
//  Created by Isak Amundsson on 2018-09-28.
//  Modified by Christian Balkenius 2020-09-17
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
    void Tick();

    Serial * s;
    char   * rcvmsg;

    float  * output;
    float  * touch;
};

#endif
