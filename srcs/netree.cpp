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

#include "netree.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace NIzihawa {
typedef __uint128_t uint128_t;

template <typename T>
inline uint32_t RadixTree<T>::getBit(T x, uint32_t bit) {
  return ((x >> (sizeof(T) * 8 - (bit))) & 1);
}

template <typename T>
RadixTree<T>::RadixNode::RadixNode(Network<T> network, bool real)
    : network(network), real(real) {}

template <typename T>
RadixTree<T>::RadixNode::~RadixNode() {
  if (children[0] != nullptr) {
    delete children[0];
  }
  if (children[1] != nullptr) {
    delete children[1];
  }
}

template <typename T>
RadixTree<T>::~RadixTree() {
  delete root;
}

template <typename T>
void RadixTree<T>::rotateNodes(RadixTree<T>::RadixNode *parentNode,
                               RadixTree<T>::RadixNode *oldNode,
                               RadixTree<T>::RadixNode *newNode) {
  if (parentNode != nullptr) {
    if (parentNode->children[0] == oldNode)
      parentNode->children[0] = newNode;
    else
      parentNode->children[1] = newNode;
  } else {
    root = newNode;
  }
}

template <typename T>
bool RadixTree<T>::mergeNodes(RadixTree<T>::RadixNode *oldNode,
                              RadixTree<T>::RadixNode *newNode) {
  if (oldNode != nullptr && newNode != nullptr &&
      oldNode->network.prefix == newNode->network.prefix &&
      oldNode->network.length == newNode->network.length) {
    oldNode->real = true;
    oldNode->network.flags = newNode->network.flags;
    delete newNode;
    return true;
  }
  return false;
}

template <typename T>
typename RadixTree<T>::LookupResult RadixTree<T>::nearest(T ip) {
  RadixTree<T>::RadixNode *parent = nullptr;
  RadixTree<T>::RadixNode *current = root;
  uint32_t flags = 0;
  while (current != nullptr) {
    if (parent != nullptr && parent->real) {
      flags |= parent->network.flags;
    }
    int prefixLength = commonPrefixLength(ip, current->network.prefix);
    if (prefixLength >= current->network.length) {
      int nextBit = getBit(ip, prefixLength + 1);
      if (current->children[nextBit] != nullptr) {
        parent = current;
        current = current->children[nextBit];
      } else {
        break;
      }
    } else {
      return RadixTree<T>::LookupResult(parent, current, flags);
    }
  }
  return RadixTree<T>::LookupResult(parent, current, flags);
}

template <typename T>
void RadixTree<T>::split(typename RadixTree<T>::RadixNode *parentNode,
                         typename RadixTree<T>::RadixNode *splitNode,
                         typename RadixTree<T>::RadixNode *newNode) {
  if (root == nullptr) {
    root = newNode;
    return;
  }
  if (mergeNodes(splitNode, newNode) || mergeNodes(parentNode, newNode)) {
    return;
  }
  int l =
      commonPrefixLength(splitNode->network.prefix, newNode->network.prefix);
  if (l < std::min(splitNode->network.length, newNode->network.length)) {
    typename RadixTree<T>::RadixNode *glueNode = new RadixTree<T>::RadixNode(
        Network<T>(
            newNode->network.prefix & ~((T(1) << (sizeof(T) * 8 - l)) - 1), l),
        false);
    rotateNodes(parentNode, splitNode, glueNode);

    int nextBit = getBit(splitNode->network.prefix, l + 1);

    glueNode->children[nextBit] = splitNode;
    glueNode->children[1 - nextBit] = newNode;

  } else {
    if (splitNode->network.length < newNode->network.length) {
      int nextBit =
          getBit(newNode->network.prefix, splitNode->network.length + 1);
      splitNode->children[nextBit] = newNode;
    } else {
      rotateNodes(parentNode, splitNode, newNode);
      int nextBit =
          getBit(splitNode->network.prefix, newNode->network.length + 1);
      newNode->children[nextBit] = splitNode;
    }
  }
}

template <typename T>
void RadixTree<T>::add(Network<T> network) {
  RadixTree<T>::RadixNode *newNode = new RadixTree<T>::RadixNode(network, true);
  auto nearestNodes = nearest(network.prefix);
  split(nearestNodes.parent, nearestNodes.node, newNode);
}

template <typename T>
typename RadixTree<T>::LookupNetwork RadixTree<T>::getNet(T ip) {
  auto nearestNodes = nearest(ip);
  uint32_t flags = nearestNodes.flags;
  if (nearestNodes.node != nullptr) {
    if ((ip ^ nearestNodes.node->network.prefix) <
        (T(1) << (sizeof(T) * 8 - nearestNodes.node->network.length - 1))) {
      flags |= nearestNodes.node->network.flags;
    }
  }
  return LookupNetwork(nearestNodes.node->network, flags);
}

template <typename T>
bool RadixTree<T>::isIn(T ip) {
  RadixTree<T>::RadixNode *nearestNode = nearest(ip).node;
  if (nearestNode != nullptr) {
    return ((ip ^ nearestNode->network.prefix) <
            (T(1) << (sizeof(T) * 8 - nearestNode->network.length - 1)));
  }
  return false;
}

template class RadixTree<uint32_t>;

template class RadixTree<uint128_t>;
};  // namespace NIzihawa