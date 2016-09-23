#include "ToLineCombination.h"

void ToLineCombination::Init()
{
  origin = GetInputArray("ORIGIN");
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

    output_matrix[0][0] = origin[0];
    output_matrix[0][1] = origin[1];
    output_matrix[0][2] = internal_matrix[0][0];
    output_matrix[0][3] = internal_matrix[0][1];
}

static InitClass init("ToLineCombination", &ToLineCombination::Create, "Source/UserModules/ToLineCombination/");
