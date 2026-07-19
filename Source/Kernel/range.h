// range.h - ranges for iteration (c) Christian Balkenius 2023

#pragma once

#include <initializer_list>
#include <string>
#include <tuple>
#include <vector>
#include <ostream>

namespace ikaros
{
    class Connection;
    struct range_access;
    
    class range
    {
    public:

        range(std::initializer_list<std::tuple<int, int, int>> ranges);

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
        [[nodiscard]] int start(int d) const;
        [[nodiscard]] int stop(int d) const;
        [[nodiscard]] int step(int d) const;
        [[nodiscard]] const std::vector<int> & index() const;   // the current index during iteration
        range & operator++();                 // advance without copying the current index
        std::vector<int> operator++(int);     // advance and return a copied index

        range & reset(int d=0);
        range & clear();
        
        [[nodiscard]] range trim() const;  // move range to 0..
        [[nodiscard]] range strip() const; // rremove dimensions with single index size
        [[nodiscard]] range tail() const;  // drop first dimension // FIXME: pop_front?

        [[nodiscard]] bool is_delay_0() const; // FIXME: rename
        [[nodiscard]] bool is_delay_1() const;

        [[nodiscard]] bool more() const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] bool empty(int d) const;
        [[nodiscard]] bool same_state(const range & other) const;

        range & set(int d, int a, int b, int inc);

        void operator=(const std::vector<int> & v);

        void print(std::string name="") const;
        void info(std::string name="") const;
        void print_index() const; // Print the current index position during a loop

        operator std::string() const;
        [[nodiscard]] std::string curly() const; // empty string or range within curly brackets

        friend bool operator==(const range & a, const range & b);
        friend bool operator!=(const range & a, const range & b);
        friend bool operator<=(const range & a, const range & b);        // is subset

        friend std::ostream & operator<<(std::ostream & os, const range & x);

    private:
        std::vector<int> inc_;
        std::vector<int> a_;
        std::vector<int> b_;
        std::vector<int> index_;

        [[nodiscard]] bool more(int d) const;
        void swap(range & other) noexcept;

        friend class Connection;
        friend struct range_access;
    };
}; // namespace ikaros
