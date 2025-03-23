#pragma once

#include <stdexcept>
#include <string>

namespace ikaros {

    class exception : public std::exception
    {
    public:
        exception(std::string message, std::string path=""): message_(message), path_(path) {};

        std::string message() const { return message_; }
        std::string path() const { return path_; }

        const char * what () const noexcept override { return message_.c_str(); }
    private:
        std::string message_;
        std::string path_;
    };

    class fatal_error : public exception 
    {
    public:
        fatal_error(std::string message, std::string path="") : exception(message, path) {}
    };

    class fatal_runtime_error : public exception 
    {
    public:
        fatal_runtime_error(std::string message, std::string path="") : exception(message, path) {}
    };


    class load_failed : public exception 
    {
    public:
        load_failed(std::string message, std::string path="") : exception(message, path) {}
    };

    class setup_error : public exception 
    {
    public:
        setup_error(std::string message, std::string path="") : exception(message, path) {}
    };

    class init_error : public exception
    {
    public:
        init_error(std::string message, std::string path="") : exception(message, path) {}
    };

    class matrix_error : public exception 
    {
    public:
        matrix_error(std::string message, std::string path="") : exception(message, path) {}
    };

    class empty_matrix_error : public matrix_error 
    {
    public:
        empty_matrix_error(std::string message, std::string path="") : matrix_error(message, path) {}
    };

    class out_of_memory_matrix_error : public matrix_error 
    {
    public:
        out_of_memory_matrix_error(std::string message, std::string path="") : matrix_error(message, path) {}
    };

    class serial_error : public exception 
    {
    public:
        serial_error(std::string message, std::string path="") : exception(message, path) {}
    };
};


