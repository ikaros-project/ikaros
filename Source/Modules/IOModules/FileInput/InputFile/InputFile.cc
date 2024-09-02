#include "ikaros.h"

using namespace ikaros;

static void
skip_comment_lines(FILE * file)
{
    char c = fgetc(file);

    while (c=='#' || c==' ' || c=='\t' || c=='\n' || c=='\r')
    {
        if (c=='#')
        {
            fscanf(file, "%*[^\n\r]");
            fscanf(file, " \r");
            fscanf(file, " \n");
        }
        else if (c==' ' || c=='\t')
        {
            fscanf(file, " \r");
            fscanf(file, " \n");
        }
        else if (c=='\n')
        {
            fscanf(file, " \n");
        }
        else if (c=='\r')
        {
            fscanf(file, " \r");
            fscanf(file, " \n");
        }
        c = fgetc(file);
    }
    ungetc(c, file);
}

class InputFile: public Module
{
    FILE	*	file;
public:
    parameter filename;
    //parameter type;
    int type;
    parameter iterations;
    parameter repetitions;
    parameter extend;
    parameter send_end_of_file;
    parameter print_iteration;

    int repetition;
    int iteration;
    int cur_extension;
    bool extending;
    int                 no_of_columns;
    int                 no_of_rows;

    std::vector<std::string>             column_name;
    std::vector<int>				column_size;
    std::vector<matrix> 			column_data;
    std::vector<matrix> static_column_data;


// Initialization must be done in creator

    InputFile()
    {
        std::string fname = GetValue("filename");
        // Bind(filename, "filename");
        std::cout << "Initfile loading fname: " << fname << "\n";
        if(fname.empty()){
             Notify(msg_fatal_error, "No input file parameter supplied.\n");
             return;
        }
        //Bind(type, "type");
        type = 0;
        

        file = fopen(fname.c_str(), "rb");
        if (file == NULL)
        {
            Notify(msg_fatal_error, "Could not open input file\n" );
            return;
        }

        no_of_columns = 0;
        char col_label[64];
        int	col_size;
        int col_ix;

        ClearOutputs();


        // Count number of columns in input file
        skip_comment_lines(file);
        int tst = fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line

        // old format:
        while (fscanf(file, "%[^/\n\r]/%d", col_label, &col_size) == 2)
        {
            no_of_columns++;
            fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
    #ifdef DEBUG_INPUTFILE
            printf("  Counting column \"%s\" with width %d in file \"%s\".\n", col_label, col_size, filename);
    #endif
        }
        
        // new format: label:0 label:1 label2:0 label2:1
        // or          label:0:0 label:0:1 label2:0 label2:1
        while (fscanf(file, "%[^/\n\r]:%d", col_label, &col_ix) == 2)
        {
            no_of_columns++;
            fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
    #ifdef DEBUG_INPUTFILE
            printf("  Counting column \"%s\" with width %d in file \"%s\".\n", col_label, col_size, filename);
    #endif
        }
        
        if(type == 0) // dynamic
        {
           
            column_name.resize(no_of_columns);
            column_size.resize(no_of_columns);
            column_data.resize(no_of_columns);
            // static_column_data = NULL;

            // for (int i=0; i<no_of_columns; i++)
            // {
            //     column_name[i] = NULL;
            //     column_data[i] = NULL;
            // }

            // Read the names and sizes of each column

            int col = 0;

            fseek(file, 0, SEEK_SET);

            skip_comment_lines(file);
            fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line

            while (fscanf(file, "%[^/\n\r]/%d", col_label, &col_size) == 2)
            {
        #ifdef DEBUG_INPUTFILE
                printf("  Finding column \"%s\" with width %d in file \"%s\".\n", col_label, col_size, filename);
        #endif
                column_name[col] = create_string(col_label);		// Allocate memory for the column name
                column_size[col] = col_size;
                // AddOutput(column_name[col], false, col_size);
                
                AddOutput(column_name[col], col_size, "programatically added output for column");
                col++;
                fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
            }
        }
        else // static
        {        
            // Allocate memory
            
            column_name.resize(no_of_columns);
            column_size.resize(no_of_columns);
            // column_data = NULL;
            static_column_data.resize(no_of_columns);
            
            // for (int i=0; i<no_of_columns; i++)
            // {
            //     column_name[i] = NULL;
            //     static_column_data[i] = NULL;
            // }
            
            // Read the names and sizes of each column
            
            int col = 0;
            
            fseek(file, 0, SEEK_SET);
            
            skip_comment_lines(file);
            fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
            
            while (fscanf(file, "%[^/\n\r]/%d", col_label, &col_size) == 2)
            {
    #ifdef DEBUG_INPUTFILE
                //	printf("  Finding column \"%s\" with width %d in file \"%s\".\n", col_label, col_size, filename);
    #endif
                column_name[col] = create_string(col_label);		// Allocate memory for the column name
                column_size[col] = col_size;
                col++;
                fscanf(file, "%*[^A-Za-z\n\r]");		// Skip until letter or end of line
            }
            
            // Count number of lines in data file
            
            int line_count = CountLines(file);
            
    #ifdef DEBUG_INPUTFILE
            printf("line_count = %d\n", line_count);
    #endif
            
            // Allocate outputs
            
            for(int j=0; j<col; j++)
                AddOutput(column_name[j], column_size[j] * line_count, "programmatically added output");

            no_of_rows = line_count;
            
            fclose(file);
        }



        // AddOutput("MYOUTPUT", 10, "my programatically added output");
    }

    virtual ~InputFile()
    {
        fclose (file);
        // delete [] column_name;
    }

    void Init()
    {
        //std::cout << "InputFile::Init" << std::endl;
        
        Bind(iterations, "iterations");
        Bind(repetitions, "repetitions");
        Bind(extend, "extend");
        Bind(send_end_of_file, "send_end_of_file");
        Bind(print_iteration, "print_iteration");
        // TODO error if filename empty
        iteration = 1;
        repetition  = repetitions;
        cur_extension = 0;
        extending = false;

        if(type == 1) // static
        {
            file = fopen(filename.string_value->c_str(), "rb");

            // Get outputs
            
            for(int j=0; j < no_of_columns; j++)
                Bind(static_column_data[j], column_name[j]);
            
            // Read data
            
            skip_comment_lines(file);
            
            fscanf(file, "%*[^\n\r]");	// Skip format line
            fscanf(file, "\r");
            fscanf(file, "\n");

            for(int j=0; j<no_of_rows; j++)
            {
                skip_comment_lines(file);
                
                for (int col=0; col < no_of_columns; col++)
                {
                    for (int i=0; i<column_size[col]; i++){
                        float n;
                        fscanf(file, "%f", &n);
                        static_column_data[col][j][i] = n;
                    }
                }
                
                fscanf(file, "%*[^\n\r]");	// Skip until end-of-line
                fscanf(file, "\r");			// Read new-line token(s)
                fscanf(file, "\n");			// Read new-line token(s)
            }
            
            fclose(file);
            
            return;
        }
        
        for (int col = 0; col < no_of_columns; col++)
            Bind(column_data[col], column_name[col]);
        
    }


    void Tick()
    {
       // std::cout << "InputFile::Tick" << std::endl;

        if(type == 1)
            return;

        if (extending)
        {
            if (cur_extension >= extend-1 && send_end_of_file)
                Notify(msg_end_of_file, filename.string_value->c_str());

            else if (cur_extension == 0)
            {
                for (int col=0; col < no_of_columns; col++)
                    for (int i=0; i<column_size[col]; i++)
                        column_data[col][i] = 0.0;
            }

            cur_extension++;
            return;
        }

        if(repetition++ < repetitions)
            return;
        else
            repetition = 1;
        
        skip_comment_lines(file);
        ReadData();

        if (feof(file))
        {
            if (print_iteration){
                std::stringstream s;
                s << "End of iteration " << iteration << "/" << iterations << "\n";
                Notify(msg_debug, s.str());
            }

            if (iteration++ < iterations)
            {
                rewind(file);
                skip_comment_lines(file);
                fscanf(file, "%*[^\n\r]");	// Skip format line
                fscanf(file, "\r");
                fscanf(file, "\n");
            }

            else if (extend == 0 && send_end_of_file){
                std::stringstream s;
                s <<  "Reached in \"" << filename << "\".\n" ;
                Notify(msg_end_of_file,s.str());
            }

            else
                extending = true;
        }
}
 

    void ReadData()
    {
        for (int col=0; col < no_of_columns; col++)
        {
            for (int i=0; i<column_size[col]; i++){
                float n;
                fscanf(file, "%f", &n);
                column_data[col][i] = n;
            }
        }
        fscanf(file, "%*[^\n\r]");	// Skip until end-of-line
        fscanf(file, "\r");			// Read new-line token(s)
        fscanf(file, "\n");			// Read new-line token(s)
    }

    void CountData()
    {
        for (int col=0; col < no_of_columns; col++)
        {
            for (int i=0; i<column_size[col]; i++)
                fscanf(file, "%*f");
        }
        fscanf(file, "%*[^\n\r]");	// Skip until end-of-line
        fscanf(file, "\r");			// Read new-line token(s)
        fscanf(file, "\n");			// Read new-line token(s)
    }

    int CountLines(FILE * file)
    {
        fseek(file, 0, SEEK_SET);
        
        // Skip first line
        
        skip_comment_lines(file);
        fscanf(file, "%*s\n");
        
        int line_count = 0; //-1;
        
        while(!feof(file))
        {
            skip_comment_lines(file);
            CountData();
            line_count++;
        }
        
        return line_count;
    }
};


INSTALL_CLASS(InputFile)

