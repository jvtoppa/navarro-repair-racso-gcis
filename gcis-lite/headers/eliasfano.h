/*
 * gcis-lite
 * Copyright (C) 2026 Racso Galvan
 *
 * This file is derived from the original GCIS implementation,
 * which is licensed under the GNU General Public License v3.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 */

#pragma once
#include <sdsl/bit_vectors.hpp>
#include <sdsl/dac_vector.hpp>
#include <sdsl/int_vector.hpp>
#include "utils.h"

#ifndef ELIASFANO_H
#define ELIASFANO_H

#endif //ELIASFANO_H

class EliasFanoDecompressedLevel {

public:
    EliasFanoDecompressedLevel() = default;
    EliasFanoDecompressedLevel(EliasFanoDecompressedLevel& rhs) = default;
    EliasFanoDecompressedLevel(EliasFanoDecompressedLevel&& rhs)  noexcept : rule(rhs.rule), rule_pos(rhs.rule_pos) { }
    ~EliasFanoDecompressedLevel() = default;

    template<typename string_type>
    void extend_rule(const uint64_t rule_num, string_type &r_string, uint64_t &l) {
        uint64_t rule_start = rule_pos[rule_num];
        uint64_t rule_bound = rule_pos[rule_num + 1];
        for (; rule_start < rule_bound; ++rule_start, ++l) {
            r_string[l] = rule[rule_start];
        }
    }

    sdsl::int_vector<> rule;
    std::vector<unsigned int> rule_pos;
};

class EliasFanoCodec {
    uint64_t m_size = 0;
    sdsl::sd_vector<> sdv;
    sdsl::sd_vector<>::select_1_type m_sel;
public:
    EliasFanoCodec() = default;
    EliasFanoCodec(EliasFanoCodec& rhs) = default;
    EliasFanoCodec(EliasFanoCodec&& rhs)  noexcept : m_size(rhs.m_size), sdv(std::move(rhs.sdv)), m_sel(rhs.m_sel) {
        sdsl::util::init_support(m_sel,&sdv);
    }
    ~EliasFanoCodec() = default;

    void encode(sdsl::int_vector<>& v) {
        for(uint64_t i = 1; i < v.size(); ++i){
            v[i] += v[i - 1] + 1;
        }
        sdv = sdsl::sd_vector<>(v.begin(), v.end());
        sdsl::util::init_support(m_sel, &sdv);
        m_size = v.size();
    }

    void encode(const sdsl::bit_vector& v) {
        sdv = sdsl::sd_vector<>(v);
        sdsl::util::init_support(m_sel, &sdv);
        m_size = v.size();
    }

    uint64_t operator[] (uint64_t i) const {
        return i == 0 ? m_sel(1) : m_sel(i + 1) - m_sel(i) - 1;
    }

    [[nodiscard]] uint64_t pos(uint64_t i)  const {
        return m_sel(i + 1);
    }

    [[nodiscard]] uint64_t access_bv(uint64_t i) const {
        return sdv[i];
    }

    static uint64_t get_next() {
        //TODO: implement sequencial scan in a efficient way
        return -1;
    }

    [[nodiscard]] uint64_t size() const {
        return m_size;
    }

    [[nodiscard]] uint64_t size_in_bytes() const {
        uint64_t total_bytes = sdsl::size_in_bytes(sdv);
        total_bytes += sdsl::size_in_bytes(m_sel);
        return total_bytes;
    }

    void serialize(std::ostream& o) const {
        o.write(reinterpret_cast<const char*>(&m_size), sizeof(m_size));
        sdv.serialize(o);
        m_sel.serialize(o);
    }

    void load(std::istream& i) {
        i.read(reinterpret_cast<char*>(&m_size), sizeof(m_size));
        sdv.load(i);
        m_sel.load(i);
        m_sel.set_vector(&sdv);
    }

    [[nodiscard]] unsigned int count() const {
        uint64_t total = 0;
        for (const auto &x : sdv) {
            total += static_cast<int>(x);
        }
        return total;
    }
};

template<int MOD>
class EliasFano {
    // string size
    uint32_t string_size{};
    // alphabet size
    uint32_t alphabet_size{};
    // elias-fano coded lcp information
    EliasFanoCodec lcp;
    // fixed-width of rule suffixes array
    sdsl::int_vector<> rule;
    // fixed-width of rule suffixes length
    EliasFanoCodec rule_suffix_length;
    // Fixed-width integer reduced string
    std::vector<uint64_t> reduced_string_ps;
    // A dac vector storing the fully decoded rule lengths
    sdsl::dac_vector_dp<> fully_decoded_rule_len;
    // A integer storing the fully decoded tail length
    uint64_t fully_decoded_tail_len{};

    std::vector<uint32_t> partial_sum;

public:

    // vector of tails
    sdsl::int_vector<> tail;

    EliasFano() = default;
    EliasFano(EliasFano &rhs) = default;
    EliasFano(EliasFano &&rhs)  noexcept = default;
    ~EliasFano() = default;

    template<typename sais_index_type, typename char_type>
    EliasFano(sais_index_type* RA,
        sais_index_type* SA,
        const char_type* T,
        sais_index_type n,
        sais_index_type k,
        sais_index_type m,
        sais_index_type names,
        int cs,
        bool is_last_level = false,
        EliasFano* prev_level = nullptr) {

        sdsl::int_vector<> lms_lengths(names, 0, sdsl::bits::hi(n) + 1);
        sdsl::int_vector<> lms_start(names, 0, sdsl::bits::hi(n) + 1);

        sais_index_type i, j, p, plen;
        sais_index_type head_section_size;
        sais_index_type c0, c1;

        head_section_size = *std::min_element(SA, SA + m);
        sais_index_type lms_position = m - 1;
        i = n - 2, j = n - 1, c0 = chr(n - 1);
        while (head_section_size <= i) {
            while (head_section_size <= i && (c1 = chr(i)) >= c0) {
                --i;
                c0 = c1;
            }
            while (head_section_size <= i && (c1 = chr(i)) <= c0) {
                --i;
                c0 = c1;
            }
            plen = j - i - (j + 1 < n);
            lms_lengths[RA[lms_position]] = plen;
            lms_start[RA[lms_position]] = i + 1;
            j = i + 1;
            --lms_position;
        }

        sdsl::int_vector<> fdrlen(names, 0, sdsl::bits::hi(n) + 1);

        sdsl::bit_vector lcp;
        sdsl::bit_vector rule_delim;

        sais_index_type final_rule_length = 0;
        sais_index_type final_lcp_length = 0;
        sais_index_type final_rule_delim_length = 0;

        for (p = 0; p < names; ++p) {
            sais_index_type cur_lcp = 0;
            i = lms_start[p];
            sais_index_type cur_len = lms_lengths[p];

            // Compute LCP of rules[i - 1] and rules[i]
            if (p > 0) {
                j = lms_start[p - 1];
                sais_index_type prev_len = lms_lengths[p - 1];
                while (cur_lcp < cur_len and cur_lcp < prev_len and chr(i) == chr(j)) {
                    ++i;
                    ++j;
                    ++cur_lcp;
                }
            }
            if (p % MOD == 0) cur_lcp = 0;
            final_rule_length += cur_len - cur_lcp;
            final_lcp_length += cur_lcp + 1;
            final_rule_delim_length += cur_len - cur_lcp + 1;
        }

        lcp.resize(final_lcp_length);
        rule_delim.resize(final_rule_delim_length);
        this -> rule.resize(final_rule_length);

        final_rule_length = 0;
        final_lcp_length = 0;
        final_rule_delim_length = 0;


        for (p = 0; p < names; ++p) {
            sais_index_type cur_lcp = 0;
            i = lms_start[p];
            sais_index_type cur_len = lms_lengths[p];

            // Compute LCP of rules[i - 1] and rules[i]
            if (p > 0) {
                j = lms_start[p - 1];
                sais_index_type prev_len = lms_lengths[p - 1];
                while (cur_lcp < cur_len and cur_lcp < prev_len and chr(i) == chr(j)) {
                    ++i;
                    ++j;
                    ++cur_lcp;
                }
            }

            if (p % MOD == 0) cur_lcp = 0;

            if (cur_lcp > 0) {
                std::fill_n(lcp.begin() + final_lcp_length, cur_lcp, false);
                final_lcp_length += cur_lcp;
            }
            if (cur_lcp < cur_len) {
                std::fill_n(rule_delim.begin() + final_rule_delim_length, cur_len - cur_lcp, false);
                final_rule_delim_length += cur_len - cur_lcp;
            }

            // Encode LCP value in unary
            lcp[final_lcp_length] = true;
            ++final_lcp_length;
            // Encode rule length in unary
            rule_delim[final_rule_delim_length] = true;
            ++final_rule_delim_length;

            i = lms_start[p] + cur_lcp;
            for (j = cur_lcp; j < cur_len; ++j, ++i) {
                this -> rule[final_rule_length] = chr(i);
                ++final_rule_length;
            }

            // Insert the fully decode rule length
            if (prev_level == nullptr) {
                // The symbols are terminal L(x) = 1, for every x
                fdrlen[p] = cur_len;
            } else {
                // The symbols are not necessarly terminal.
                sais_index_type rule_ptr = lms_start[p];
                sais_index_type sum =
                    prev_level -> fully_decoded_rule_len[chr(rule_ptr)];
                ++rule_ptr;
                for (j = 1; j < cur_len; ++j, ++rule_ptr) {
                    sum +=
                        prev_level -> fully_decoded_rule_len[chr(rule_ptr)];
                }
                fdrlen[p] = sum;
            }
        }
        sdsl::util::clear(lms_start);
        sdsl::util::clear(lms_lengths);

        sdsl::util::bit_compress(this -> rule);
        this -> lcp.encode(lcp);
        this -> rule_suffix_length.encode(rule_delim);
        this -> fully_decoded_rule_len = sdsl::dac_vector_dp<>(fdrlen);
        sdsl::util::clear(fdrlen);
        sdsl::util::clear(lcp);
        sdsl::util::clear(rule_delim);

        this -> tail.resize(head_section_size);
        for (i = 0; i < head_section_size; ++i) {
            this -> tail[i] = chr(i);
        }
        // Compress the tail
        sdsl::util::bit_compress(this -> tail);
        // Determine the accumulated tail length
        if (prev_level != nullptr) {
            this -> fully_decoded_tail_len =
                prev_level -> fully_decoded_tail_len;
            for (auto t : this -> tail) {
                this -> fully_decoded_tail_len +=
                    prev_level -> fully_decoded_rule_len[t];
            }
        } else {
            this -> fully_decoded_tail_len = this -> tail.size();
        }
        this -> string_size = n;
        this -> alphabet_size = k;
        if (is_last_level) {
            this -> partial_sum.resize(m);
            if (not this -> partial_sum.empty()) {
                this -> partial_sum[0] = 0;
            }
            for (i = 1; i < m; ++i) {
                this -> partial_sum[i] =
                    this -> partial_sum[i - 1] +
                    this -> fully_decoded_rule_len[RA[i - 1]];
            }
        }
    }

    [[nodiscard]] uint64_t size_in_bytes() const {
        uint64_t total_bytes = 0;
        total_bytes += 2 * sizeof(uint32_t);
        total_bytes += lcp.size_in_bytes();
        total_bytes += rule_suffix_length.size_in_bytes();
        total_bytes += sdsl::size_in_bytes(rule);
        total_bytes += sdsl::size_in_bytes(tail);
        return total_bytes;
    }

    void serialize(std::ostream& out) const {
        out.write(reinterpret_cast<const char*>(&string_size), sizeof(string_size));
        out.write(reinterpret_cast<const char*>(&alphabet_size), sizeof(alphabet_size));
        lcp.serialize(out);
        rule_suffix_length.serialize(out);
        rule.serialize(out);
        tail.serialize(out);
        fully_decoded_rule_len.serialize(out);
        out.write(reinterpret_cast<const char*>(&fully_decoded_tail_len),
                  sizeof(fully_decoded_tail_len));
    };

    void load(std::istream& in) {
        in.read(reinterpret_cast<char*>(&string_size), sizeof(string_size));
        in.read(reinterpret_cast<char*>(&alphabet_size), sizeof(alphabet_size));
        lcp.load(in);
        rule_suffix_length.load(in);
        rule.load(in);
        tail.load(in);
        fully_decoded_rule_len.load(in);
        in.read(reinterpret_cast<char*>(&fully_decoded_tail_len),
                sizeof(fully_decoded_tail_len));
    }

    uint32_t get_string_size() const {
        return this -> string_size;
    }

    uint32_t get_alphabet_size() const {
        return this -> alphabet_size;
    }

    EliasFanoDecompressedLevel decompress() {
        EliasFanoDecompressedLevel decompressed;
        uint64_t number_of_rules = rule_suffix_length.count();
        decompressed.rule_pos.resize(number_of_rules + 1);
        uint64_t total_length = 0;
        // Compute the total LCP length
        for (uint64_t i = 0; i < number_of_rules; ++i) {
            uint64_t cur_length = lcp[i] + rule_suffix_length[i];
            total_length += cur_length;
            decompressed.rule_pos[i] = cur_length;
        }

        // Resize data structures

        decompressed.rule.resize(total_length);
        decompressed.rule.width(sdsl::bits::hi(alphabet_size - 1) + 1);

        uint64_t rule_start = 0;
        uint64_t prev_rule_start = 0;
        uint64_t start = 0;

        for (uint64_t i = 0; i < number_of_rules; ++i) {
            uint64_t lcp_length = lcp[i];
            total_length = decompressed.rule_pos[i];

            // Copy the contents of the previous rule by LCP length chars
            uint64_t j = 0;
            while (j < lcp_length) {
                decompressed.rule[rule_start + j] = decompressed.rule[prev_rule_start + j];
                ++j;
            }

            // Copy the remaining suffix rule
            while (j < total_length) {
                decompressed.rule[rule_start + j] = rule[start];
                ++start;
                ++j;
            }

            decompressed.rule_pos[i] = rule_start;
            prev_rule_start = rule_start;
            rule_start += total_length;
        }
        decompressed.rule_pos[number_of_rules] = rule_start;
        return decompressed;
    }
};
