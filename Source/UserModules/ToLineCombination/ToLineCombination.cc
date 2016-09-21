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

void ToLineCombination::SetSizes() {
  output_rows = 0;
  for(int i=0; i<input_matrix_size_x;i++){
    if(internal_matrix[i][0] != -1){
      output_rows++;
    }else{
      break;
    }
  }
  SetOutputSize("OUTPUT", output_rows, 4);
}

void ToLineCombination::Tick() {
  SetSizes();
  copy_matrix(internal_matrix, input_matrix, input_matrix_size_x, input_matrix_size_y);

  for (int i=0; i<output_rows; i++){
          output_matrix[i][0] = x0;
          output_matrix[i][1] = y0;
          output_matrix[i][2] = internal_matrix[i][0];
          output_matrix[i][3] = internal_matrix[i][1];
        }

}

static InitClass init("ToLineCombination", &ToLineCombination::Create, "Source/UserModules/ToLineCombination/");
