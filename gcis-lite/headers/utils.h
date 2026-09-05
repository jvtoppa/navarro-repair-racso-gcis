/*
 * gcis-lite
 * Copyright (C) 2026 Racso Galvan
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 */

#pragma once
#include <fstream>

#define MALLOC_VARIABLE(var, _size, _type) \
    ((var) = (_type*)malloc((_size) * sizeof(_type)))

#define MALLOC_VARIABLE2(var1, var2, _size, _type) \
    (((var1) = (_type*)malloc((_size) * sizeof(_type))) != nullptr && \
     ((var2) = (_type*)malloc((_size) * sizeof(_type))) != nullptr ? (var1) : nullptr)

#define SAIS_MYFREE(_ptr, _num, _type) \
    do { free((_ptr)); (_ptr) = nullptr; } while(0)
#define chr(_a) (cs == sizeof(sais_index_type) ? ((sais_index_type *)T)[(_a)] : ((unsigned char *)T)[(_a)])

#ifndef UTILS_H
#define UTILS_H

#endif //UTILS_H

inline void print_usage_message() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  ./a.out <mode> <input file> <output filename> <encoder type>" << std::endl;
}

inline void load_string_from_file(char *&str, const char *filename, int &string_size) {
    std::ifstream f(filename, std::ios::binary);
    f.seekg(0, std::ios::end);
    const std::streamsize n = f.tellg();
    string_size = static_cast<int>(n);
    f.seekg(0, std::ios::beg);

    str = new char[n];
    f.read(str, n);
};

template<typename string_type>
bool check_printable(const string_type &s) {
    for (auto x : s) {
        if (static_cast<int>(x) < 32) return false;
        if (static_cast<int>(x) > 126) return false;
    }
    return true;
}