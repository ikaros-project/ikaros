#ifndef EXCEPTIONS
#define EXCEPTIONS

namespace ikaros {

    class exception : public std::exception
    {
    public:
        std::string message;
        std::string path; // Path to module or parameter with problem
        exception(std::string msg): message(msg) {};
        const char * what () const throw () { return message.c_str(); }
    };

    class fatal_error : public exception 
    {
    public:
        fatal_error(std::string msg) : exception(msg) {}
    };

    class fatal_runtime_error : public exception 
    {
    public:
        fatal_runtime_error(std::string msg) : exception(msg) {}
    };


    class load_failed : public exception 
    {
    public:
        load_failed(std::string msg) : exception(msg) {}
    };

    class setup_error : public exception 
    {
    public:
        setup_error(std::string msg) : exception(msg) {}
    };

    class init_error : public exception
    {
    public:
        init_error(std::string msg) : exception(msg) {}
    };

    class empty_matrix_error : public exception 
    {
    public:
        empty_matrix_error(std::string msg) : exception(msg) {}
    };
};

#endif

/*

PLANNED EXCEPTIONS

    load_failed: connot continue, call new
    setup_error: cannot set up network, run server oinly
    init_error: modules could not be initialized
    fatal_runtime_error: cannot continue running "simulation", run-time error
    fatal_error: cabbot cannot at all: try do deallocate and quit


*/