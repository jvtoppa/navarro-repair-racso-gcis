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

#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include <sdsl/bit_vectors.hpp>
#include "utils.h"

template <typename sais_index_type, typename encoder_type>
class GrammarInterface {

    std::vector<encoder_type> g;
    sdsl::int_vector<> final_reduced_string;

public:

    virtual ~GrammarInterface() = default;

    GrammarInterface()
        : g() {}

    GrammarInterface(std::vector<encoder_type> grammar)
        : g(std::move(grammar)) {}

    GrammarInterface(const GrammarInterface&) = default;
    GrammarInterface(GrammarInterface&&) = default;
    GrammarInterface& operator=(const GrammarInterface&) = default;
    GrammarInterface& operator=(GrammarInterface&&) = default;

    [[nodiscard]] uint64_t size_in_bytes() const {
        uint64_t total_bytes = 0;

        for (const auto& level : g)
            total_bytes += level.size_in_bytes();

        total_bytes += sdsl::size_in_bytes(final_reduced_string);
        return total_bytes;
    }

    template<typename char_type>
    void add_rule_level(
        sais_index_type* RA,
        sais_index_type* SA,
        const char_type* T,
        sais_index_type n,
        sais_index_type k,
        sais_index_type m,
        int cs,
        bool is_last_level
    ) {

        if (m == 0) return;

        sais_index_type names = *std::max_element(RA, RA + m) + 1;

        if (g.empty()) {
            //std::cout << "Empty case" << std::endl;
            g.emplace_back(RA, SA, T, n, k, m, names, cs, is_last_level);
        }
        else {
            //std::cout << "Non-empty case" << std::endl;
            g.emplace_back(RA, SA, T, n, k, m, names, cs, is_last_level, &g.back());
        }
    }

    template<typename char_type>
    void set_reduced_string(const char_type* RA, sais_index_type m) {
        final_reduced_string.resize(m);

        for (sais_index_type i = 0; i < m; ++i)
            final_reduced_string[i] = RA[i];

        //sdsl::util::bit_compress(final_reduced_string);
    }

    void serialize_rules(std::ostream& o) const {
        if (!o)
            throw std::runtime_error("Error writing rules");

        uint64_t size = g.size();
        o.write(reinterpret_cast<const char*>(&size), sizeof(size));

        for (const auto& level : g)
            level.serialize(o);
    }

    void serialize_reduced_string(std::ostream& o) const {
        if (!o)
            throw std::runtime_error("Error writing reduced string");
        final_reduced_string.serialize(o);
    }


    void load_rules(std::istream& i) {
        uint64_t size;
        i.read(reinterpret_cast<char*>(&size), sizeof(size));

        if (!i)
            throw std::runtime_error("Error reading grammar rules size");

        g.resize(size);
        for (uint64_t j = 0; j < size; ++j)
            g[j].load(i);
    }

    void load_reduced_string(std::istream& i) {
            final_reduced_string.load(i);
            if (!i)
                throw std::runtime_error("Error reading reduced string");
        }

    void decode(std::ostream& out) {
    sdsl::int_vector<> r_string(final_reduced_string.size());
    for (sais_index_type i = 0; i < final_reduced_string.size(); ++i) {
        r_string[i] = final_reduced_string[i];
    }

    if (!g.empty()) {
        for (int64_t i = g.size() - 1; i >= 0; --i) {
            auto decompressed_encoder = std::move(g[i].decompress());

            if (i == 0) {
                // Ensure tail size + rule expansions fit in allocation
                const uint64_t n = g[i].get_string_size();
                char *str = new char[n];
                uint64_t new_r_string_ptr = 0;

                for (auto c : g[i].tail) {
                    if (new_r_string_ptr < n) {
                        str[new_r_string_ptr++] = c;
                    }
                }

                for (auto c : r_string) {
                    decompressed_encoder.extend_rule(c, str, new_r_string_ptr);
                }

                out.write(str, n);
                delete[] str; // Free buffer
            } 
            else {
                const uint64_t target_size = g[i].get_string_size();
                const uint8_t width = sdsl::bits::hi(g[i].get_alphabet_size()) + 1;
                
                sdsl::int_vector<> new_r_string(target_size, 0, width);
                uint64_t new_r_string_ptr = 0;

                for (auto c : g[i].tail) {
                    if (new_r_string_ptr < target_size) {
                        new_r_string[new_r_string_ptr++] = c;
                    }
                }

                for (auto c : r_string) {
                    if (new_r_string_ptr >= target_size) {
                        throw std::runtime_error("grammar decode: expansion exceeds expected level length");
                    }
                    decompressed_encoder.extend_rule(c, new_r_string, new_r_string_ptr);
                }

                r_string = std::move(new_r_string);
            }
        }
    } 
    else {
        char *str = new char[final_reduced_string.size()];
        for (uint64_t i = 0; i < final_reduced_string.size(); i++) {
            str[i] = static_cast<char>(final_reduced_string[i]);
        }
        out.write(str, final_reduced_string.size());
        delete[] str;
    }
}

};
