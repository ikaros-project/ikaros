#include "ToLineCombination.h"

void ToLineCombination:Init()
{
  origin = GetInputArray("ORIGIN");
  x0 = origin[0];
  y0 = origin[1];
}

void ToLineCombination::Tic() {
  point = GetInputMatrix("INPUT");
  output_matrix_length = GetInputSizeX("INPUT");
  output_matrix = create_matrix(output_matrix_length,4);

  for (int i=0; i<output_matrix_length; i++){
          output_matrix[i][0] = x0;
          output_matrix[i][1] = y1;
          output_matrix[i][2] = point[i][0];
          output_matrix[i][3] = point[i][1];
        }

  output_matrix = "OUTPUT";
}

static InitClass init("ToLineCombination", &ToLineCombination::Create, "Source/UserModules/ToLineCombination/");
