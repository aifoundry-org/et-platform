/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_DUMP_DATA_H
#define BEMU_DUMP_DATA_H

#include <cstddef>
#include <fstream>
#include <ostream>
#include "agent.h"

namespace bemu {


template<typename Container>
void dump_data(std::ostream& os, const Container& cont, std::size_t pos, std::size_t n,
               const typename Container::value_type& default_value)
{
    using value_type = typename Container::value_type;
    using size_type  = typename Container::size_type;

    size_type m = (pos >= cont.size()) ? 0 : std::min(n, cont.size() - pos);
    if (m > 0) {
        const char* source = reinterpret_cast<const char*>(cont.data() + pos);
        os.write(source, m * sizeof(value_type));
        n -= m;
    }
    value_type zero(default_value);
    while (n-- > 0)
        os.write(reinterpret_cast<const char*>(&zero), sizeof(value_type));
}


template<typename Container>
void dump_data(const Container& cont, const Agent& agent, const char* filename,
               unsigned long long addr, std::size_t n)
{
    std::ofstream file;
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    file.open(filename, std::ios::out);
    cont.dump_data(agent, file, addr, n);
}


} // namespace bemu

#endif // BEMU_DUMP_DATA_H
