// range.h - ranges for iteration (c) Christian Balkenius 2023

#pragma once

#include <initializer_list>
#include <string>
#include <tuple>
#include <vector>
#include <ostream>

namespace ikaros
{
    
    class range
    {
    public:

        range(std::initializer_list<std::tuple<int, int, int>> ranges);

        std::vector<int>    inc_;   // FIXME: change to vector of individual ranges instead for clearer code
        std::vector<int>    a_;
        std::vector<int>    b_;
        std::vector<int>    index_;

        range();
        range(int a);
        range(int a, int b, int inc=1);
        range(const std::string & s);                            // parse range string: [a:b:inc]

        range & push(int a, int b, int inc=1);
        range & push(int a);
        range & push();

        range & push_front(int a, int b, int inc=1);
        range & extend(int n);              // Extend the range to n dimensions
        range & extend(const range & r);          // Extend size to include r
        range & fill(const range & r);      // Fill empty ranges from r

        [[nodiscard]] int rank() const;                         // dimensionality of the range
        [[nodiscard]] int size() const;                         // number of elements in the range
        [[nodiscard]] int size(int f) const;                    // number of element in odimension d of the range
        [[nodiscard]] std::vector<int> extent() const;          // the largest index in each fimension
        std::vector<int> & index() ;        // the current index during iteration
        std::vector<int> operator++(int);

        range & reset(int d=0);
        range & clear();
        
        [[nodiscard]] range trim() const;  // move range to 0..
        [[nodiscard]] range strip() const; // rremove dimensions with single index size
        [[nodiscard]] range tail() const;  // drop first dimension // FIXME: pop_front?

        [[nodiscard]] bool is_delay_0() const; // FIXME: rename
        [[nodiscard]] bool is_delay_1() const;

        [[nodiscard]] bool more(int d=0) const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] bool empty(int d) const;

        operator std::vector<int> &();

        range & set(int d, int a, int b, int inc);

        void operator=(const std::vector<int> & v);

        friend void operator|=(range & r, range & s);

        void print(std::string name="") const;
        void info(std::string name="") const;
        void print_index() const; // Print the current index position during a loop

        operator std::string() const;
        [[nodiscard]] std::string curly() const; // empty string or range within curly brackets

        friend bool operator==(const range & a, const range & b);
        friend bool operator!=(const range & a, const range & b);
        friend bool operator<=(const range & a, const range & b);        // is subset

        friend std::ostream& operator<<(std::ostream& os, const range & x);
    };
}; // namespace ikaros
