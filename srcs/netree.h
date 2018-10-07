/*
MIT License
Copyright 2018 Pasha Perevedentsev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <algorithm>
#include <arpa/inet.h>

#include "netree.h"

namespace NIzihawa {
typedef __uint128_t uint128_t;

// Returns rightmost set bit
inline int clz(uint128_t u) {
  uint64_t hi = u >> 64;
  uint64_t lo = u;
  int retval[3] = {__builtin_clzll(hi), __builtin_clzll(lo) + 64, 128};
  int idx = !hi + ((!lo) & (!hi));
  return retval[idx];
}

inline int clz(uint32_t u) { return __builtin_clz(u); }

template <typename T>
struct Network {
  T prefix;
  uint32_t length;
  uint32_t flags;
  Network(T prefix, uint32_t length, uint32_t flags = 0)
      : prefix(prefix), length(length), flags(flags) {}
};

template <typename T>
class RadixTree {
 private:
  struct RadixNode {
    Network<T> network;
    RadixNode *children[2] = {nullptr, nullptr};
    bool real;
    RadixNode(Network<T> network, bool real = false);
    ~RadixNode();
  };

  struct LookupResult {
    RadixTree<T>::RadixNode *parent;
    RadixTree<T>::RadixNode *node;
    uint32_t flags;
    LookupResult(RadixTree<T>::RadixNode *parent, RadixTree<T>::RadixNode *node,
                 uint32_t flags = 0)
        : parent(parent), node(node), flags(flags) {}
  };

  RadixTree<T>::RadixNode *root = nullptr;

  inline uint32_t getBit(T x, uint32_t bit);
  void rotateNodes(RadixTree<T>::RadixNode *parentNode,
                   RadixTree<T>::RadixNode *oldNode,
                   RadixTree<T>::RadixNode *newNode);
  bool mergeNodes(RadixTree<T>::RadixNode *oldNode,
                  RadixTree<T>::RadixNode *newNode);
  LookupResult nearest(T ip);
  void split(RadixTree<T>::RadixNode *parentNode,
             RadixTree<T>::RadixNode *splitNode,
             RadixTree<T>::RadixNode *newNode);
  inline int commonPrefixLength(T a, T b) { return clz(a ^ b); }

 public:
  struct LookupNetwork {
    Network<T> network;
    uint32_t flags;
    LookupNetwork(Network<T> network, uint32_t flags)
        : network(network), flags(flags) {}
  };

  ~RadixTree();

  void add(Network<T> network);
  LookupNetwork getNet(T ip);
  bool isIn(T ip);
};

class Netree {
 private:
  RadixTree<uint32_t> ipv4tree;
  RadixTree<uint128_t> ipv6tree;

 public:
};
}  // namespace NIzihawa
