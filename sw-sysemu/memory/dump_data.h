/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_DUMP_DATA_H
#define BEMU_DUMP_DATA_H

#include <cstddef>
#include <fstream>
#include <ostream>

namespace bemu {


template<typename Container>
void dump_data(std::ostream& os, const Container& cont, size_t pos, size_t n)
{
    typedef typename Container::value_type value_type;
    typedef typename Container::size_type size_type;

    size_type m = (pos >= cont.size()) ? 0 : std::min(n, cont.size() - pos);
    if (m > 0) {
        const char* source = reinterpret_cast<const char*>(cont.data() + pos);
        os.write(source, m * sizeof(value_type));
        n -= m;
    }
    value_type zero = value_type();
    while (n-- > 0)
        os.write(reinterpret_cast<const char*>(&zero), sizeof(value_type));
}


template<typename Container>
void dump_data(const Container& cont, const char* filename,
               unsigned long long addr, size_t n)
{
    std::ofstream file;
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    file.open(filename, std::ios::out);
    cont.dump_data(file, addr, n);
}


} // namespace bemu

#endif // BEMU_DUMP_DATA_H
