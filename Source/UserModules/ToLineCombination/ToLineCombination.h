#ifndef ToLineCombination
#define ToLineCombination

#include "IKAROS.h"

class ToLineCombination: public Module
{

public:
	//Have not added parameters yet
	static Module * Create(Parameter * p) { return new ToLineCombination(p); }

	ToLineCombination(Parameter * p) : Module(p) {}
	virtual ~ToLineCombination();

	void Init();
	void Tick();

	int[][] OUTPUT;

	};

#endif
