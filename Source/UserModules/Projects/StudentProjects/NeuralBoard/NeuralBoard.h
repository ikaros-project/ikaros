#ifndef TouchBoard_
#define TouchBoard_

#include "IKAROS.h"
#include "stdio.h"

class NeuralBoard: public Module
{
public:
  NeuralBoard(Parameter * p);
  virtual ~NeuralBoard();
  static Module *Create(Parameter * p);
  void Init();
  void Tick();

  float * output;
};
#endif /* NeuralBoard_H*/
