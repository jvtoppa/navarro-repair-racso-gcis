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

#ifndef SIMPLE8B_H
#define SIMPLE8B_H

#endif //SIMPLE8B_H

class simple8b_selector {
public:
    uint32_t n_items;
    uint32_t n_bits;
    uint32_t n_waste;
};

static simple8b_selector s8b_selector[16] = {
    {240, 0, 60},
    {120, 0, 60},
    {60, 1, 0},
    {30, 2, 0},
    {20,  3, 0},{15, 4, 0},{12, 5, 0},
    {10, 6, 0},
    {8, 7, 4},
    {7, 8, 4},
    {6,10,0},
    {5,12,0},
    {4,15,0},
    {3,20,0},
    {2,30,0},
    {1,60,0}
};

class Simple8bDecompressedLevel {

public:
    Simple8bDecompressedLevel() = default;
    Simple8bDecompressedLevel(Simple8bDecompressedLevel& rhs) = default;
    Simple8bDecompressedLevel(Simple8bDecompressedLevel&& rhs)  noexcept : rule(rhs.rule), rule_pos(rhs.rule_pos) { }
    ~Simple8bDecompressedLevel() = default;

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

template<uint64_t BUF_SIZE = 4800>
class Simple8BCodec {
    std::vector<uint64_t> m_v;
    // cache to store integers to be coded and decoded
    uint64_t m_buf[BUF_SIZE] = {};
    uint64_t m_buf_i = 0;
    // Number of integers coded
    uint64_t m_size = 0;
    // Number of integers decoded
    uint64_t m_buf_size = 0;
    // Current word to be extracted from m_v
    uint64_t m_cur_word = 0;



    // Flushes the content in buffer to the final vector
    // Returns the number of itens remaining in the buffer
    uint64_t flush(uint64_t buf_size) {
        uint64_t index = pack(buf_size);
        /* We reached the last element of the buffer but we do not know if this is the correct selector. We need to look to the other elements */
        if(index < BUF_SIZE){
            // copy elements to the buffer start
            for(uint64_t k = 0;k < BUF_SIZE - index; ++k){
                m_buf[k] = m_buf[index + k];
            }
        }
        return BUF_SIZE - index;
    }

    uint64_t decode_to_buffer() {
        uint64_t index = 0;
        while(m_cur_word < m_v.size()){
            uint64_t word = m_v[m_cur_word];
            uint64_t n_items = s8b_selector[word & 0xf].n_items;
            uint64_t n_bits =  s8b_selector[word & 0xf].n_bits;
            word >>= 4;
            if(index + n_items > BUF_SIZE) {
                break;
            }
            for(uint64_t i = 0; i < n_items; ++i){
                m_buf[index++] = word & sdsl::bits::lo_set[n_bits];
                word >>= n_bits;
            }
            m_cur_word++;
        }
        return index;
    }

    // Pack a series of integers.
    // Returns the number of packed integers
    uint64_t pack(uint64_t buf_size) {
        uint64_t index = 0;
        while(index < buf_size) {
            for(short i = 0; i < 16; ++i) {
                uint64_t word = i;
                uint64_t shift = 4;
                uint64_t n_items = 0;
                for(uint64_t j = index; j < buf_size; ++j){
                    if(n_items == s8b_selector[i].n_items) {
                        break;
                    }
                    if(m_buf[j] > ((1ULL << s8b_selector[i].n_bits) - 1)) {
                        break;
                    }
                    word |=  m_buf[j] << shift;
                    shift += s8b_selector[i].n_bits;
                    n_items++;
                }

                // We have a filled a word with selector 'i'
                if(n_items == s8b_selector[i].n_items) {
                    m_v.push_back(word);
                    index += n_items;
                    break;
                }
            }
        }
        return index;
    }

    uint64_t pack() {
        uint64_t index = 0;
        while(index < m_buf_i){
            for(short i = 0; i < 16; ++i){
                uint64_t word = i;
                uint64_t shift = 4;
                uint64_t n_items = 0;
                for(uint64_t j = index; j < m_buf_i; ++j){
                    if(n_items == s8b_selector[i].n_items) {
                        break;
                    }
                    if(m_buf[j] > ((1ULL << s8b_selector[i].n_bits) - 1)) {
                        break;
                    }
                    word |=  m_buf[j] << shift;
                    shift += s8b_selector[i].n_bits;
                    n_items++;
                }

                // We have a filled a word with selector 'i'
                if((n_items == s8b_selector[i].n_items) || (index + n_items == m_buf_i)) {
                    m_v.push_back(word);
                    index += n_items;
                    break;
                }
            }
        }
        m_buf_i = 0;
        return index;
    }

public:

    // Encode a entire array in simple8b
    void encode(uint64_t* v, uint64_t n) {
        //TODO: implement
    }

    // Encode a single integer in simple8b
    void encode(uint64_t k) {
        m_buf[m_buf_i++] = k;
        if(m_buf_i == BUF_SIZE) {
            m_buf_i = flush(BUF_SIZE);
        }
        m_size++;
    }

    // Finish the encoding process
    void encode() {
        pack();
    }

    uint64_t get_next() {
        if(m_buf_i == m_buf_size) {
            m_buf_size = decode_to_buffer();
            m_buf_i = 0;
        }
        return m_buf[m_buf_i++];
    }

    [[nodiscard]] uint64_t size() const {
        return m_size;
    }

    void reset() {
        m_buf_i = 0;
        m_buf_size = 0;
        m_cur_word = 0;
    }

    [[nodiscard]] uint64_t size_in_bytes() const {
        uint64_t total_bytes = m_v.size() * sizeof(uint64_t);
        total_bytes += (5 + BUF_SIZE) * sizeof(uint64_t);
        return total_bytes;
    }

    void serialize(std::ostream& o) const {
        const uint64_t size = m_v.size();
        o.write(reinterpret_cast<const char*>(&m_size),sizeof(uint64_t));
        o.write(reinterpret_cast<const char*>(&size),sizeof(uint64_t));
        o.write(reinterpret_cast<const char*>(m_v.data()),sizeof(uint64_t)*size);
    }

    void load(std::istream& i) {
        uint64_t size;
        i.read(reinterpret_cast<char*>(&m_size),sizeof(uint64_t));
        i.read(reinterpret_cast<char*>(&size),sizeof(uint64_t));
        m_v.resize(size);
        i.read(reinterpret_cast<char*>(m_v.data()),sizeof(uint64_t) * size);
    }
};

template<int MOD>
class S8B {
    uint32_t string_size{};
    uint32_t alphabet_size{};
    Simple8BCodec<> lcp;
    Simple8BCodec<> rule_suffix_length;
    sdsl::int_vector<> rule;


public:

    sdsl::int_vector<> tail;

    S8B() = default;
    S8B(S8B &rhs) = default;
    S8B(S8B &&rhs) noexcept = default;

    template<typename sais_index_type, typename char_type>
    S8B(sais_index_type* RA,
        sais_index_type* SA,
        const char_type* T,
        sais_index_type n,
        sais_index_type k,
        sais_index_type m,
        sais_index_type names,
        int cs,
        bool is_last_level = false, S8B* prev_level = nullptr) {
        sdsl::int_vector<> lms_lengths(names);
        sdsl::int_vector<> lms_start(names);

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
            //std::cout << i + 1 << " " << j << std::endl;
            lms_lengths[RA[lms_position]] = plen;
            lms_start[RA[lms_position]] = i + 1;
            j = i + 1;
            --lms_position;
        }

        //std::cout << "Starting new level of Simple8b" << std::endl;
        //std::cout << "Names: " << names << std::endl;*/

        sais_index_type final_rule_length = 0;

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
            /*std::cout << p << ": ";
            for (j = 0; j < cur_len; ++j) {
                std::cout << static_cast<int>(chr(lms_start[p] + j)) << " ";
            }
            std::cout << std::endl;*/
        }

        //std::cout << "Computed final lengths" << std::endl;
        this -> rule.resize(final_rule_length);

        final_rule_length = 0;

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
            //std::cout << p << " " << cur_lcp << std::endl;
            if (p % MOD == 0) cur_lcp = 0;

            //std::cout << "Resized lcp and rule_delim" << std::endl;

            this -> lcp.encode(cur_lcp);
            this -> rule_suffix_length.encode(cur_len - cur_lcp);

            //std::cout << "Filled lcp and rule_delim" << std::endl;

            i = lms_start[p] + cur_lcp;
            for (j = cur_lcp; j < cur_len; ++j, ++i) {
                this -> rule[final_rule_length] = chr(i);
                ++final_rule_length;
            }

            //std::cout << "Extended rule" << std::endl;
        }
        sdsl::util::clear(lms_start);
        sdsl::util::clear(lms_lengths);
        //std::cout << "Computed lcps for compression" << std::endl;
        //std::cout << "Generated rules" << std::endl;

        //std::cout << fdrlen.size() << std::endl;

        //std::cout << "Finished added rules" << std::endl;

        //std::cout << "Bit compressing rules" << std::endl;

        sdsl::util::bit_compress(this -> rule);
        //std::cout << "Compressed rules" << std::endl;
        this -> lcp.encode();
        //std::cout << "Encoded lcp" << std::endl;
        this -> rule_suffix_length.encode();
        //std::cout << "Encoded rule_delim" << std::endl;

        //std::cout << "Compressing the tail" << std::endl;

        this -> tail.resize(head_section_size);
        for (i = 0; i < head_section_size; ++i) {
            this -> tail[i] = chr(i);
        }
        // Compress the tail
        sdsl::util::bit_compress(this -> tail);
        this -> string_size = n;
        this -> alphabet_size = k;
        //std::cout << "Finished this level" << std::endl;*/
    }

    uint64_t size_in_bytes() const {
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
    };

    void load(std::istream& in) {
        in.read(reinterpret_cast<char*>(&string_size), sizeof(string_size));
        in.read(reinterpret_cast<char*>(&alphabet_size), sizeof(alphabet_size));
        lcp.load(in);
        rule_suffix_length.load(in);
        rule.load(in);
        tail.load(in);
    }

    uint32_t get_string_size() const {
        return this -> string_size;
    }

    uint32_t get_alphabet_size() const {
        return this -> alphabet_size;
    }

    Simple8bDecompressedLevel decompress() {
        Simple8bDecompressedLevel decompressed;
        uint64_t number_of_rules = rule_suffix_length.size();
        decompressed.rule_pos.resize(number_of_rules + 1);
        uint64_t total_length = 0;
        // Compute the total LCP length
        lcp.reset();
        rule_suffix_length.reset();
        for (uint64_t i = 0; i < number_of_rules; ++i) {
            uint64_t cur_length = lcp.get_next() + rule_suffix_length.get_next();
            total_length += cur_length;
            decompressed.rule_pos[i] = cur_length;
        }

        // Resize data structures

        decompressed.rule.width(sdsl::bits::hi(alphabet_size - 1) + 1);
        decompressed.rule.resize(total_length);

        uint64_t rule_start = 0;
        uint64_t prev_rule_start = 0;
        uint64_t start = 0;
        lcp.reset();

        for (uint64_t i = 0; i < number_of_rules; ++i) {

            uint64_t lcp_length = lcp.get_next();
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