#include "ToLineCombination.h"

void ToLineCombination:Init()
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

void ToLineCombination::Tick() {
  copy_matrix(internal_matrix, input_matrix, input_matrix_size_x, input_matrix_size_y);
  int output_lenght = 0;
  for(int i=0; i<input_matrix_size_x;i++){
    if(internal_matrix[i][0] != -1){
      output_lenght++;
    }else{
      break;
    }
  }


  output_matrix_length = GetInputSizeX("INPUT");
  output_matrix = create_matrix(output_lenght,4);
s
  for (int i=0; i<output_lenght; i++){
          output_matrix[i][0] = x0;
          output_matrix[i][1] = y1;
          output_matrix[i][2] = internal_matrix[i][0];
          output_matrix[i][3] = internal_matrix[i][1];
        }

}

static InitClass init("ToLineCombination", &ToLineCombination::Create, "Source/UserModules/ToLineCombination/");
