//
// Created by racso on 1/03/2026.
//

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <iostream>

#include "simple8b.h"

template<int MOD, typename char_type, typename sais_index_type>
S8B<MOD> build_level(
    std::vector<sais_index_type>& RA,
    std::vector<sais_index_type>& SA,
    std::vector<char_type>& T,
    sais_index_type n,
    sais_index_type k,
    sais_index_type m,
    sais_index_type names
) {
    int cs = sizeof(char_type);

    S8B<MOD> level(
        RA.data(),
        SA.data(),
        T.data(),
        n,
        k,
        m,
        names,
        cs,
        true,
        nullptr
    );

    return level;
}

TEST(Simple8BCodecTest, random) {
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(1, 1 << 10);
    std::vector<uint64_t> v;
    Simple8BCodec<> s8;
    for(uint64_t i = 0; i < (1 << 20); ++i){
        uint64_t n = dis(gen);
        v.push_back(n);
        s8.encode(n);
    }
    s8.encode();
    for(uint64_t i = 0; i < s8.size(); ++i){
        EXPECT_EQ(v[i], s8.get_next());
    }
    //    s8.reset();
    //    for(uint64_t i=0;i<s8.size();i++){
    //        EXPECT_EQ(v[i],s8.get_next());
    //    }
}

TEST(Simple8BCodecTest, encode_decode_sequence) {
    Simple8BCodec<> codec;

    std::vector<uint64_t> values = {
        1, 2, 3, 4, 5, 10, 15, 31, 63, 127,
        255, 1023, 4095, 65535
    };

    for (auto v : values) {
        codec.encode(v);
    }

    codec.encode(); // finalize

    codec.reset();

    for (size_t i = 0; i < values.size(); ++i) {
        EXPECT_EQ(codec.get_next(), values[i]);
    }

    EXPECT_EQ(codec.size(), values.size());
}

TEST(Simple8BCodecTest, serialize_load_consistency) {
    Simple8BCodec<> codec;

    std::vector<uint64_t> values = {1,3,7,15,31,63,127,255};

    for (auto v : values) {
        codec.encode(v);
    }

    codec.encode();

    std::stringstream ss;
    codec.serialize(ss);

    Simple8BCodec<> loaded;
    loaded.load(ss);

    loaded.reset();

    for (size_t i = 0; i < values.size(); ++i) {
        EXPECT_EQ(loaded.get_next(), values[i]);
    }

    EXPECT_EQ(codec.size(), loaded.size());
}

TEST(Simple8BCodecSerializeTest, random){
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(1, 1 << 10);
    std::vector<uint64_t> v;
    Simple8BCodec<> s8;
    std::ofstream o;
    o.open("teste.bin",std::ofstream::binary);
    for(uint64_t i = 0; i < (1 << 20); ++i){
        uint64_t n = dis(gen);
        v.push_back(n);
        s8.encode(n);
    }
    s8.encode();
    s8.serialize(o);
    Simple8BCodec<> s8_prime;
    std::ifstream i;
    i.open("teste.bin",std::ifstream::binary);
    s8_prime.load(i);
    for(uint64_t i = 0; i < (1 << 20); ++i) {
        EXPECT_EQ(v[i], s8_prime.get_next());
    }
}

TEST(Simple8BTest, basic_build) {
    std::string text = "banana";
    using char_type = uint8_t;
    using index_type = uint32_t;

    std::vector<char_type> T(text.begin(), text.end());
    index_type n = T.size();
    index_type k = 256;

    std::vector<index_type> SA = {3,1,5,0,2,4};
    std::vector<index_type> RA = {1, 0};

    index_type m = 2;
    index_type names = 2;

    auto level = build_level<1, char_type,index_type>(
        RA, SA, T, n, k, m, names
    );

    EXPECT_GT(level.size_in_bytes(), 0);
    EXPECT_EQ(level.get_string_size(), n);
    EXPECT_EQ(level.get_alphabet_size(), k);
}

TEST(Simple8BTest, serialize_load_consistency) {
    std::string text = "banana";
    using char_type = uint8_t;
    using index_type = uint32_t;

    std::vector<char_type> T(text.begin(), text.end());
    index_type n = T.size();
    index_type k = 256;

    std::vector<index_type> SA = {3,1,5,0,2,4};
    std::vector<index_type> RA = {1, 0};

    index_type m = 2;
    index_type names = 2;

    auto level = build_level<1, char_type,index_type>(
        RA, SA, T, n, k, m, names
    );

    std::stringstream ss;
    level.serialize(ss);

    S8B<1> loaded;
    loaded.load(ss);

    EXPECT_EQ(level.get_string_size(), loaded.get_string_size());
    EXPECT_EQ(level.get_alphabet_size(), loaded.get_alphabet_size());
    EXPECT_EQ(level.size_in_bytes(), loaded.size_in_bytes());
}

TEST(Simple8BTest, decompress_consistency) {
    std::string text = "AGCTTTTCATTCTGACTGCAACAGCTTTTCATTCTGACTGCAAC";
    using char_type = uint8_t;
    using index_type = uint32_t;

    std::vector<char_type> T(text.begin(), text.end());
    index_type n = T.size();
    index_type k = 256;

    std::vector<index_type> SA = {
        41, 19, 14, 36, 22, 8, 30, 11, 33, 2, 24, 0,
        7, 0, 0, 5, 6, 0, 3, 0, 2, 0, 4, 7,
        0, 0, 5, 6, 0, 3, 0, 1, 0, 6, 4, 5,
        2, 1, 3, 6, 4, 5, 2, 0
    };

    std::vector<index_type> RA = {
        6, 4, 5, 2, 1, 3, 6, 4, 5, 2, 0
    };

    index_type m = RA.size();
    index_type names = *std::max_element(RA.begin(), RA.end()) + 1;

    auto level = build_level<1, char_type,index_type>(
        RA, SA, T, n, k, m, names
    );

    auto decompressed = level.decompress();

    EXPECT_GT(decompressed.rule_pos.size(), 0);

    for (size_t i = 1; i < decompressed.rule_pos.size(); ++i) {
        EXPECT_LE(
            decompressed.rule_pos[i - 1],
            decompressed.rule_pos[i]
        );
    }

    EXPECT_EQ(
        decompressed.rule_pos[decompressed.rule_pos.size() - 1],
        decompressed.rule.size()
    );
}