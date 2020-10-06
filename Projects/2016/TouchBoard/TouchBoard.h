//
//  TouchBoard.h
//
//
//  Created by Isak Amundsson on 2018-09-28.
//

#ifndef TouchBoard_
#define TouchBoard_

#include "IKAROS.h"
#include "stdio.h"
class TouchBoard: public Module
{
public:
    Serial *s;
    char    *rcvmsg;
    TouchBoard(Parameter * p);
    virtual ~TouchBoard();
    static Module *Create(Parameter * p);
    void Init();
    void PrintValue();
    void Tick();


    float *	output;
};
#endif /* TouchBoard_h*/
