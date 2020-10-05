#include "NeuralBoard.h"
#include "IKAROS.h"
#include "../IOModules/FileOutput/OutputFile/OutputFile.h"
#include "../IOModules/FileInput/InputFile/InputFile.h"

using namespace ikaros;

Module * NeuralBoard::Create(Parameter * p){
  return new NeuralBoard(p);

}


NeuralBoard::NeuralBoard(Parameter * p ): Module(p){
  //input = new InputFile(InputExample.txt)
}

NeuralBoard::~NeuralBoard(){

}

void NeuralBoard::Init(){
    //output = new OutputFile(OutputExample.txt)
}

void NeuralBoard::Tick(){

}

static InitClass init("NeuralBoard", &NeuralBoard::Create, "Source/Modules/NeuralBoard/");
