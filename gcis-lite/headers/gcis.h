/*
 * Portions of this file are derived from sais-lite.
 *
 * sais-lite:
 * Copyright (c) 2008-2009 Yuta Mori
 * Released under the MIT License.
 *
 * Modifications and integration:
 * Copyright (C) 2026 Racso Galvan
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is distributed as part of gcis-lite under
 * the GNU General Public License v3 or later.
 */

#pragma once
#include "grammar.h"
#include "utils.h"

constexpr int MINBUCKETSIZE = 256;
constexpr int SAIS_LMSSORT2_LIMIT = 0x3fffffff;

template<typename char_type, typename sais_index_type>
static
void
get_counts(const char_type *T, sais_index_type *C, sais_index_type n, sais_index_type k, int cs) {
  sais_index_type i;
  for(i = 0; i < k; ++i) { C[i] = 0; }
  for(i = 0; i < n; ++i) { ++C[chr(i)]; }
}

template<typename sais_index_type>
static
void
get_buckets(const sais_index_type *C, sais_index_type *B, sais_index_type k, bool end) {
  sais_index_type i, sum = 0;
  if(end) { for(i = 0; i < k; ++i) { sum += C[i]; B[i] = sum; } }
  else { for(i = 0; i < k; ++i) { sum += C[i]; B[i] = sum - C[i]; } }
}

template<typename char_type, typename sais_index_type>
static
void
LMSsort1(const char_type *T, sais_index_type *SA,
         sais_index_type *C, sais_index_type *B,
         sais_index_type n, sais_index_type k, int cs) {
  sais_index_type *b, i, j;
  sais_index_type c0, c1;
  /* compute SAl */
  if(C == B) { get_counts<char_type, sais_index_type>(T, C, n, k, cs); }
  get_buckets<sais_index_type>(C, B, k, false); /* find starts of buckets */
  j = n - 1;
  b = SA + B[c1 = chr(j)];
  --j;
  *b++ = (chr(j) < c1) ? ~j : j;
  for(i = 0; i < n; ++i) {
    if(0 < (j = SA[i])) {
      assert(chr(j) >= chr(j + 1));
      if((c0 = chr(j)) != c1) { B[c1] = b - SA; b = SA + B[c1 = c0]; }
      assert(i < (b - SA));
      --j;
      *b++ = (chr(j) < c1) ? ~j : j;
      SA[i] = 0;
    } else if(j < 0) {
      SA[i] = ~j;
    }
  }
  /* compute SAs */
  if(C == B) { get_counts<char_type, sais_index_type>(T, C, n, k, cs); }
  get_buckets<sais_index_type>(C, B, k, true); /* find ends of buckets */
  for(i = n - 1, b = SA + B[c1 = 0]; 0 <= i; --i) {
    if(0 < (j = SA[i])) {
      assert(chr(j) <= chr(j + 1));
      if((c0 = chr(j)) != c1) { B[c1] = b - SA; b = SA + B[c1 = c0]; }
      assert((b - SA) <= i);
      --j;
      *--b = (chr(j) > c1) ? ~(j + 1) : j;
      SA[i] = 0;
    }
  }
}
template<typename char_type, typename sais_index_type, typename sais_bool_type>
static
sais_index_type
LMSpostproc1(const char_type *T, sais_index_type *SA,
             sais_index_type n, sais_index_type m, int cs) {
  sais_index_type i, j, p, q, plen, qlen, name;
  sais_index_type c0, c1;
  sais_bool_type diff;
  /* compact all the sorted substrings into the first m items of SA
      2*m must be not larger than n (proveable) */
  assert(0 < n);
  for(i = 0; (p = SA[i]) < 0; ++i) { SA[i] = ~p; assert((i + 1) < n); }
  if(i < m) {
    for(j = i, ++i;; ++i) {
      assert(i < n);
      if((p = SA[i]) < 0) {
        SA[j++] = ~p; SA[i] = 0;
        if(j == m) { break; }
      }
    }
  }

  /* store the length of all substrings */
  i = n - 1; j = n - 1; c0 = chr(n - 1);
  do { c1 = c0; } while((0 <= --i) && ((c0 = chr(i)) >= c1));
  for ( ; 0 <= i; ) {
    do { c1 = c0; } while((0 <= --i) && ((c0 = chr(i)) <= c1));
    if(0 <= i) {
      SA[m + ((i + 1) >> 1)] = j - i; j = i + 1;
      do { c1 = c0; } while((0 <= --i) && ((c0 = chr(i)) >= c1));
    }
  }

  /* find the lexicographic names of all substrings */
  for(i = 0, name = 0, q = n, qlen = 0; i < m; ++i) {
    p = SA[i], plen = SA[m + (p >> 1)], diff = 1;
    if((plen == qlen) && ((q + plen) < n)) {
      for(j = 0; (j < plen) && (chr(p + j) == chr(q + j)); ++j) { }
      if(j == plen) { diff = 0; }
    }
    if(diff != 0) { ++name, q = p, qlen = plen; }
    SA[m + (p >> 1)] = name;
  }

  return name;
}

template<typename char_type, typename sais_index_type>
static
void
LMSsort2(const char_type *T, sais_index_type *SA,
         sais_index_type *C, sais_index_type *B, sais_index_type *D,
         sais_index_type n, sais_index_type k, int cs) {
  sais_index_type *b, i, j, t, d;
  sais_index_type c0, c1;
  assert(C != B);
  /* compute SAl */
  get_buckets<sais_index_type>(C, B, k, false); /* find starts of buckets */
  j = n - 1;
  b = SA + B[c1 = chr(j)];
  --j;
  t = (chr(j) < c1);
  j += n;
  *b++ = (t & 1) ? ~j : j;
  for(i = 0, d = 0; i < n; ++i) {
    if(0 < (j = SA[i])) {
      if(n <= j) { d += 1; j -= n; }
      assert(chr(j) >= chr(j + 1));
      if((c0 = chr(j)) != c1) { B[c1] = b - SA; b = SA + B[c1 = c0]; }
      assert(i < (b - SA));
      --j;
      t = c0; t = (t << 1) | (chr(j) < c1);
      if(D[t] != d) { j += n; D[t] = d; }
      *b++ = (t & 1) ? ~j : j;
      SA[i] = 0;
    } else if(j < 0) {
      SA[i] = ~j;
    }
  }
  for(i = n - 1; 0 <= i; --i) {
    if(0 < SA[i]) {
      if(SA[i] < n) {
        SA[i] += n;
        for(j = i - 1; SA[j] < n; --j) { }
        SA[j] -= n;
        i = j;
      }
    }
  }

  /* compute SAs */
  get_buckets<sais_index_type>(C, B, k, true); /* find ends of buckets */
  for(i = n - 1, d += 1, b = SA + B[c1 = 0]; 0 <= i; --i) {
    if(0 < (j = SA[i])) {
      if(n <= j) { d += 1; j -= n; }
      assert(chr(j) <= chr(j + 1));
      if((c0 = chr(j)) != c1) { B[c1] = b - SA; b = SA + B[c1 = c0]; }
      assert((b - SA) <= i);
      --j;
      t = c0; t = (t << 1) | (chr(j) > c1);
      if(D[t] != d) { j += n; D[t] = d; }
      *--b = (t & 1) ? ~(j + 1) : j;
      SA[i] = 0;
    }
  }
}

template<typename sais_index_type>
static
sais_index_type
LMSpostproc2(sais_index_type *SA, sais_index_type n, sais_index_type m) {
  sais_index_type i, j, d, name;

  /* compact all the sorted LMS substrings into the first m items of SA */
  assert(0 < n);
  for(i = 0, name = 0; (j = SA[i]) < 0; ++i) {
    j = ~j;
    if(n <= j) { name += 1; }
    SA[i] = j;
    assert((i + 1) < n);
  }
  if(i < m) {
    for(d = i, ++i;; ++i) {
      assert(i < n);
      if((j = SA[i]) < 0) {
        j = ~j;
        if(n <= j) { name += 1; }
        SA[d++] = j; SA[i] = 0;
        if(d == m) { break; }
      }
    }
  }
  //std::cout << name << " " << m << std::endl;
    /* store the lexicographic names */
  //if (name < m) {
    for(i = m - 1, d = name + 1; 0 <= i; --i) {
      if(n <= (j = SA[i])) { j -= n; --d; }
      SA[m + (j >> 1)] = d;
      //std::cout << "LMS substring " << j << " has name " << d << std::endl;
    }
  /*} else {
    // unset flags
    for(i = 0; i < m; ++i) {
      if(n <= (j = SA[i])) { j -= n; SA[i] = j; }
    }
  }*/

  return name;
}

template<typename char_type, typename sais_index_type, typename grammar_encoder_type>
static
sais_index_type
gcis_main(const char_type *T, sais_index_type *SA, sais_index_type fs, sais_index_type n, sais_index_type k, int cs, GrammarInterface<sais_index_type, grammar_encoder_type> &grammar_encoder, int lvl, int final_level)
{
  sais_index_type *C, *B, *D, *RA, *b;
  sais_index_type i, j, m, p, q, t, name, pidx = 0, newfs;
  sais_index_type c0, c1;
  unsigned int flags;

  if (T == nullptr || SA == nullptr || fs < 0 || n <= 0 || k < 1) {
      return -1;
  }

  if(k <= MINBUCKETSIZE) {
    if(MALLOC_VARIABLE(C, k, sais_index_type) == nullptr) { return -2; }
    if(k <= fs) {
      B = SA + (n + fs - k);
      flags = 1;
    } else {
      if(MALLOC_VARIABLE(B, k, sais_index_type) == nullptr) { SAIS_MYFREE(C, k, sais_index_type); return -2; }
      flags = 3;
    }
  } else if(k <= fs) {
    C = SA + (n + fs - k);
    if(k <= (fs - k)) {
      B = C - k;
      flags = 0;
    } else if(k <= (MINBUCKETSIZE * 4)) {
      if(MALLOC_VARIABLE(B, k, sais_index_type) == nullptr) { return -2; }
      flags = 2;
    } else {
      B = C;
      flags = 8;
    }
  } else {
    if(MALLOC_VARIABLE2(C, B, k, sais_index_type) == nullptr) { return -2; }
    flags = 4 | 8;
  }
  if((n <= SAIS_LMSSORT2_LIMIT) && (2 <= (n / k))) {
    if(flags & 1) { flags |= ((k * 2) <= (fs - k)) ? 32 : 16; }
    else if((flags == 0) && ((k * 2) <= (fs - k * 2))) { flags |= 32; }
  }

  /* stage 1: reduce the problem by at least 1/2
     sort all the LMS-substrings */
  get_counts<char_type, sais_index_type>(T, C, n, k, cs);
  get_buckets<sais_index_type>(C, B, k, true); /* find ends of buckets */
  std::fill(SA, SA + n, 0);
  b = &t; i = n - 1; j = n; m = 0; c0 = chr(n - 1);
  do { c1 = c0; } while((0 <= --i) && ((c0 = chr(i)) >= c1));
  for(; 0 <= i;) {
    do { c1 = c0; } while((0 <= --i) && ((c0 = chr(i)) <= c1));
    if(0 <= i) {
      *b = j; b = SA + --B[c1]; j = i; ++m;
      do { c1 = c0; } while((0 <= --i) && ((c0 = chr(i)) >= c1));
    }
  }

  if(1 < m) {
    if(flags & (16 | 32)) {
      if(flags & 16) {
        if(MALLOC_VARIABLE(D, k * 2, sais_index_type) == nullptr) {
          if(flags & (1 | 4)) { SAIS_MYFREE(C, k, sais_index_type); }
          if(flags & 2) { SAIS_MYFREE(B, k, sais_index_type); }
          return -2;
        }
      } else {
        D = B - k * 2;
      }
      assert((j + 1) < n);
      ++B[chr(j + 1)];
      for(i = 0, j = 0; i < k; ++i) {
        j += C[i];
        if(B[i] != j) { assert(SA[B[i]] != 0); SA[B[i]] += n; }
        D[i] = D[i + k] = 0;
      }
      LMSsort2<char_type, sais_index_type>(T, SA, C, B, D, n, k, cs);
      name = LMSpostproc2<sais_index_type>(SA, n, m);
      if(flags & 16) { SAIS_MYFREE(D, k * 2, sais_index_type); }
    } else {
      LMSsort1<char_type, sais_index_type>(T, SA, C, B, n, k, cs);
      name = LMSpostproc1<char_type, sais_index_type, bool>(T, SA, n, m, cs);
    }
  } else if(m == 1) {
    *b = j + 1;
    name = 1;
  } else {
    name = 0;
  }



  /* stage 2: solve the reduced problem
     recurse if names are not yet unique */

  if(flags & 4) { SAIS_MYFREE(C, k, sais_index_type); }
  if(flags & 2) { SAIS_MYFREE(B, k, sais_index_type); }
  newfs = (n + fs) - (m * 2);
  if((flags & (1 | 4 | 8)) == 0) {
    if((k + name) <= newfs) { newfs -= k; }
    else { flags |= 8; }
  }
  assert((n >> 1) <= (newfs + m));
  RA = SA + m + newfs;
  for(i = m + (n >> 1) - 1, j = m - 1; m <= i; --i) {
    if(SA[i] != 0) {
      RA[j--] = SA[i] - 1;
    }
  }
  if (flags & (16 | 32)) {
    for (i = 0; i < m; ++i) {
      if (SA[i] >= n) { SA[i] -= n; }
    }
  }
  if(lvl < final_level) {
    if(m <= 0) {
        grammar_encoder.template add_rule_level<char_type>(RA, SA, T, n, k, m, cs, true);
        grammar_encoder.template set_reduced_string<sais_index_type>(RA, m);
        if(flags & (1 | 4)) { SAIS_MYFREE(C, k, sais_index_type); }
        if(flags & 2) { SAIS_MYFREE(B, k, sais_index_type); }
        return -1;
    }
    grammar_encoder.template add_rule_level<char_type>(RA, SA, T, n, k, m, cs, false);    
    if(gcis_main<sais_index_type, sais_index_type, grammar_encoder_type>(RA, SA, newfs, m, name, sizeof(sais_index_type), grammar_encoder, lvl + 1, final_level) != 0) {
      if(flags & 1) { SAIS_MYFREE(C, k, sais_index_type); }
      return -2;
    }
    if(flags & 4) {
      if(MALLOC_VARIABLE2(C, B, k, int) == nullptr) { return -2; }
    }
    if(flags & 2) {
      if(MALLOC_VARIABLE(B, k, int) == nullptr) {
        if(flags & 1) { SAIS_MYFREE(C, k, sais_index_type); }
        return -2;
      }
    }
  }
  else {
    grammar_encoder.template add_rule_level<char_type>(RA, SA, T, n, k, m, cs, true);
    grammar_encoder.template set_reduced_string<sais_index_type>(RA, m);
  }

  if(flags & (1 | 4)) { SAIS_MYFREE(C, k, sais_index_type); }
  if(flags & 2) { SAIS_MYFREE(B, k, sais_index_type); }

  return pidx;
}


template<typename char_type, typename sais_index_type, typename grammar_encoder_type>
int gcis(GrammarInterface<sais_index_type, grammar_encoder_type> &grammar_encoder, const char_type* T, sais_index_type *SA, int n, int k = -1, int final_level = 0) {
    if((T == nullptr) || (SA == nullptr) || (n < 0)) { return -1; }
    if(n <= 1) { if(n == 1) { SA[0] = 0; } return -1; }

    int level = 0;
    return gcis_main(T, SA, 0, n, ~k ? k : 256, sizeof(char_type), grammar_encoder, level, final_level);
}