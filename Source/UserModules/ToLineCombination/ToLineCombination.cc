#include "ToLineCombination.h"

void ToLineCombination::Init()
{
  origin = GetInputArray("ORIGIN");
  x0 = origin[0];
  y0 = origin[1];
  output_matrix = GetOutputMatrix("OUTPUT");
  input_matrix = GetInputMatrix("INPUT");
  input_matrix_size_x = GetInputSizeX("INPUT");
  input_matrix_size_y = GetInputSizeY("INPUT");
  internal_matrix = create_matrix(input_matrix_size_x, input_matrix_size_y);
}

ToLineCombination::~ToLineCombination()
{
    destroy_matrix(internal_matrix);
}

void ToLineCombination::Tick() {
  copy_matrix(internal_matrix, input_matrix, input_matrix_size_x, input_matrix_size_y);

    output_matrix[0][0] = x0;
    output_matrix[0][1] = y0;
    output_matrix[0][2] = internal_matrix[i][0];
    output_matrix[0][3] = internal_matrix[i][1];
}

static InitClass init("ToLineCombination", &ToLineCombination::Create, "Source/UserModules/ToLineCombination/");
