#ifndef ToLineCombination
#define ToLineCombination

#include "IKAROS.h"

class ToLineCombination: public Module
{

public:
	//Have not added parameters yet
	static Module * Create() { 
		return new ToLineCombination(); 
	}
	
	ToLineCombination() : Module() {};
	virtual ~ToLineCombination() {};
	
	void Init();
	void Tick();
	
	int	size;

	};

#endif