#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <iostream>

#include "eliasfano.h"

// =====================================================
// Helper: Construye un EliasFano<32> nivel base
// =====================================================

template<typename char_type, typename sais_index_type>
EliasFano<32> build_level(
    std::vector<sais_index_type>& RA,
    std::vector<sais_index_type>& SA,
    std::vector<char_type>& T,
    sais_index_type n,
    sais_index_type k,
    sais_index_type m,
    sais_index_type names
) {
    int cs = sizeof(char_type);

    EliasFano<32> level(
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

TEST(EliasFanoCodecTest, int_vector_access) {
    sdsl::int_vector<> v = {1, 0, 0, 1, 2, 0, 0, 4, 4, 0};
    sdsl::int_vector<> v2 = v;
    EliasFanoCodec ef;
    ef.encode(v);
    for(uint64_t i = 0; i < v.size(); ++i){
        EXPECT_EQ(ef[i], v2[i]);
    }
}


TEST(EliasFanoCodecTest, bitvector_access) {
    sdsl::bit_vector bv = {1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1};
    EliasFanoCodec ef;
    ef.encode(bv);
    EXPECT_EQ(ef.size(), bv.size());
    for(uint64_t i = 0; i < bv.size(); ++i){
        EXPECT_EQ(ef.access_bv(i), bv[i]);
    }
}

TEST(EliasFanoCodecTest, bitvector_sel) {
    sdsl::bit_vector bv = {1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1};
    EliasFanoCodec ef;
    ef.encode(bv);
    EXPECT_EQ(ef.size(), bv.size());
    uint64_t k = 0;
    for(uint64_t i = 0; i < bv.size(); ++i){
        if(bv[i]) {
            EXPECT_EQ(ef.pos(k), i);
            k++;
        }
    }
}

TEST(EliasFanoCodecTest, bitvector_value) {
    sdsl::bit_vector bv = {1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1};
    sdsl::int_vector<> v = {0, 2, 0, 1, 3, 0, 2};
    EliasFanoCodec ef;
    ef.encode(bv);
    EXPECT_EQ(ef.size(), bv.size());
    for(uint64_t i = 0; i < v.size(); ++i){
        EXPECT_EQ(ef[i], v[i]);
    }
}

TEST(EliasFanoCodecSerializeTest, bitvector_access) {
    std::ofstream o("/tmp/teste.dat");
    sdsl::bit_vector bv = {1,0,0,1,1,0,0,1,0,1,0,1,0,1};
    EliasFanoCodec ef;
    ef.encode(bv);
    ef.serialize(o);
    EliasFanoCodec ef2;
    o.close();
    std::ifstream i("/tmp/teste.dat");
    ef2.load(i);
    for(uint64_t i = 0; i < bv.size(); ++i){
        EXPECT_EQ(ef2.access_bv(i), bv[i]);
    }
    i.close();
}

TEST(EliasFanoCodecSerializeTest, bitvector_sel) {
    std::ofstream o("/tmp/teste.dat");
    sdsl::bit_vector bv = {1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1};
    EliasFanoCodec ef;
    ef.encode(bv);
    ef.serialize(o);
    EliasFanoCodec ef2;
    o.close();
    std::ifstream i("/tmp/teste.dat");
    ef2.load(i);
    uint64_t k = 0;
    for(uint64_t i = 0; i < bv.size(); ++i){
        if(bv[i]) {
            EXPECT_EQ(ef2.pos(k), i);
            k++;
        }
    }
    i.close();
}

TEST(EliasFanoTest, basic_build) {
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

    auto level = build_level<char_type,index_type>(
        RA, SA, T, n, k, m, names
    );

    EXPECT_GT(level.size_in_bytes(), 0);
    EXPECT_EQ(level.get_string_size(), n);
    EXPECT_EQ(level.get_alphabet_size(), k);
}

TEST(EliasFanoTest, serialize_load_consistency) {
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

    auto level = build_level<char_type,index_type>(
        RA, SA, T, n, k, m, names
    );

    std::stringstream ss;
    level.serialize(ss);

    EliasFano<32> loaded;
    loaded.load(ss);

    EXPECT_EQ(level.get_string_size(), loaded.get_string_size());
    EXPECT_EQ(level.get_alphabet_size(), loaded.get_alphabet_size());
    EXPECT_EQ(level.size_in_bytes(), loaded.size_in_bytes());
}

TEST(EliasFanoTest, decompress_consistency) {
    std::string text = "AGCTTTTCATTCTGACTGCAACAGCTTTTCATTCTGACTGCAAC";
    using char_type = uint8_t;
    using index_type = uint32_t;

    std::vector<char_type> T(text.begin(), text.end());
    index_type n = T.size();
    index_type k = 256;

    std::vector<index_type> SA = {41, 19, 14, 36, 22, 8, 30, 11, 33, 2, 24, 0, 7, 0, 0, 5, 6, 0, 3, 0, 2, 0, 4, 7, 0, 0, 5, 6, 0, 3, 0, 1, 0, 6, 4, 5, 2, 1, 3, 6, 4, 5, 2, 0};
    std::vector<index_type> RA = {6, 4, 5, 2, 1, 3, 6, 4, 5, 2, 0};

    index_type m = RA.size();
    index_type names = *std::max_element(RA.begin(), RA.end()) + 1;

    auto level = build_level<char_type,index_type>(
        RA, SA, T, n, k, m, names
    );

    auto decompressed = level.decompress();

    EXPECT_GT(decompressed.rule_pos.size(), 0);

    for (size_t i = 1; i < decompressed.rule_pos.size(); ++i) {
        EXPECT_LE(decompressed.rule_pos[i - 1],
                  decompressed.rule_pos[i]);
    }

    EXPECT_EQ(
        decompressed.rule_pos[decompressed.rule_pos.size() - 1],
        decompressed.rule.size()
    );
}