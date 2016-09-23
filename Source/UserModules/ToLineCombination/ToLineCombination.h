#ifndef ToLineCombination_
#define ToLineCombination_

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
	int x0;
	int y0;
	float ** output_matrix;
	int input_matrix_size_x;
	int input_matrix_size_y;
	float ** internal_matrix;
	float ** input_matrix;
	float * origin;

	};

#endif
