// range.h - ranges for iteration (c) Christian Balkenius 2023

#ifndef RANGE
#define RANGE

#include <string>
#include <vector>

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
        range(std::string s);                            // parse range string: [a:b:inc]

        range & push(int a, int b, int inc=1);
        range & push(int a);
        range & push();

        range & push_front(int a, int b, int inc=1);
        range & extend(int n);              // Extend the range to n dimensions
        range & extend(const range & r);          // Extend size to include r
        range & fill(const range & r);      // Fill empty ranges from r

        int rank() const;                         // dimensionality of the range
        int size();                         // number of elements in the range
        int size(int f);                    // number of element in odimension d of the range
        std::vector<int> extent();          // the largest index in each fimension
        std::vector<int> & index() ;        // the current index during iteration
        std::vector<int> operator++(int);

        range & reset(int d=0);
        range & clear();
        
        range trim();  // move range to 0..
        range strip(); // rremove dimensions with single index size
        range tail();  // drop first dimension // FIXME: pop_front?

        bool is_delay_0(); // FIXME: rename
        bool is_delay_1();

        bool more(int d=0) const;
        bool empty() const;
        bool empty(int d) const;

        operator std::vector<int> &();

        range & set(int d, int a, int b, int inc);

        void operator=(const std::vector<int> & v);

        friend void operator|=(range & r, range & s);

        void print(std::string name="");
        void info(std::string name="");
        void print_index(); // Print the current index position during a loop

        operator std::string();
        std::string curly(); // empty string or range within curly brackets

        friend bool operator==(range & a, range & b);
        friend bool operator!=(range & a, range & b);
        friend bool operator<=(range & a, range & b);        // is subset

        friend std::ostream& operator<<(std::ostream& os, const range & x);
    };
}; // namespace ikaros

#endif

