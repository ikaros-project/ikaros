//
//  TouchBoard.cpp
//
//
//  Created by Isak Amundsson on 2018-09-28.
//

#include "TouchBoardGraph.h"
#include "Ikaros.h"
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>

using namespace ikaros;

Module * TouchBoardGraph::Create(Parameter * p){
    return new TouchBoardGraph(p);
}

TouchBoardGraph::TouchBoardGraph(Parameter * p ): Module(p){
    AddOutput("OUTPUT", false, 1000);
}

TouchBoardGraph::~TouchBoardGraph(){

}

void TouchBoardGraph::Init(){
    output			= GetOutputArray("OUTPUT");
}

void TouchBoardGraph::Tick() {

}


static InitClass init("TouchBoardGraph", &TouchBoardGraph::Create, "Source/Modules/TouchBoard/TouchBoardGraph");
