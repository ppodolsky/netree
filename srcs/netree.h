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
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <util/system/types.h>

typedef __uint128_t uint128_t;

/*
 * Returns rightmost set bit
 */
inline ui32 clz(uint128_t u) {
    ui64 hi = u >> 64;
    ui64 lo = u;
    i32 retval[3] = {__builtin_clzll(hi), __builtin_clzll(lo) + 64, 128};
    ui32 idx = !hi + ((!lo) & (!hi));
    return (ui32)retval[idx];
}

inline ui32 clz(ui32 u) {
    return __builtin_clz(u);
}

template <typename T>
struct Network {
    T prefix;
    ui32 length;
    ui32 flags;
    Network(T prefix, ui32 length, ui32 flags)
        : prefix(prefix)
        , length(length)
        , flags(flags)
    {
        if (length > sizeof(T) * 8) {
            throw std::runtime_error("Length should be less or equal than prefix width");
        }
    }
    Network()
        : prefix(0)
        , length(0)
        , flags(0)
    {
    }
    bool hasIp(const T ip) const {
        if (length == 0) {
            return true;
        }
        return bool((ip ^ prefix) < (T(1) << (sizeof(T) * 8 - length)));
    }
    std::string toString() const {
        T copiedPrefix = this->prefix;
        std::stringstream resultStream;
        resultStream << ((copiedPrefix >> ((sizeof(copiedPrefix) - 1) * 8)) & 0xff);
        for (ui32 i = 1; i < sizeof(copiedPrefix); ++i) {
            resultStream << ".";
            resultStream << ((copiedPrefix >> ((sizeof(copiedPrefix) - 1 - i) * 8)) & 0xff);
        }
        resultStream << "/" << this->length;
        return resultStream.str();
    }
};

template <>
std::string Network<uint128_t>::toString() const {
    uint128_t copiedPrefix = this->prefix;
    std::stringstream resultStream;
    resultStream << std::hex;
    resultStream << std::setfill('0') << std::setw(2) << (ui64)((copiedPrefix >> ((sizeof(copiedPrefix) - 1) * 8) & 0xff));
    resultStream << std::setfill('0') << std::setw(2) << (ui64)((copiedPrefix >> ((sizeof(copiedPrefix) - 2) * 8) & 0xff));
    for (ui32 i = 2; i < sizeof(copiedPrefix); i += 2) {
        resultStream << ":";
        resultStream << std::setfill('0') << std::setw(2) << (ui64)((copiedPrefix >> ((sizeof(copiedPrefix) - 1 - i) * 8)) & 0xff);
        resultStream << std::setfill('0') << std::setw(2) << (ui64)((copiedPrefix >> ((sizeof(copiedPrefix) - 2 - i) * 8)) & 0xff);
    }
    resultStream << "/" << std::dec << this->length;
    return resultStream.str();
}

template <typename T>
class NetworkTree {
private:
    struct RadixNode {
        Network<T> network;
        std::shared_ptr<RadixNode> children[2] = {nullptr, nullptr};
        bool real = false;
        RadixNode(Network<T> network, bool real = false)
            : network(network)
            , real(real)
        {
        }
    };

    struct LookupResult {
        std::shared_ptr<NetworkTree<T>::RadixNode> parent = nullptr;
        std::shared_ptr<NetworkTree<T>::RadixNode> node = nullptr;
        ui32 flags = 0;
        LookupResult(std::shared_ptr<NetworkTree<T>::RadixNode> parent,
                     std::shared_ptr<NetworkTree<T>::RadixNode> node,
                     ui32 flags)
            : parent(parent)
            , node(node)
            , flags(flags)
        {
        }
        LookupResult() = default;
    };

    std::shared_ptr<NetworkTree<T>::RadixNode> root;

    inline ui32 getBit(const T x, const ui32 bit) const {
        if (!bit || bit > sizeof(T) * 8) {
            throw std::runtime_error(
                "Bit position " + std::to_string(bit) +
                " should be positive and within the number width " + std::to_string(sizeof(T) * 8));
        }
        return ((x >> (sizeof(T) * 8 - (bit))) & 1);
    }
    void rotateNodes(std::shared_ptr<NetworkTree<T>::RadixNode> parentNode,
                     std::shared_ptr<NetworkTree<T>::RadixNode> oldNode,
                     std::shared_ptr<NetworkTree<T>::RadixNode> newNode) {
        if (parentNode != nullptr) {
            if (parentNode->children[0] == oldNode)
                parentNode->children[0] = newNode;
            else
                parentNode->children[1] = newNode;
        } else {
            root = newNode;
        }
    }
    bool mergeNodes(std::shared_ptr<NetworkTree<T>::RadixNode> oldNode,
                    std::shared_ptr<NetworkTree<T>::RadixNode> newNode) {
        if (oldNode != nullptr && newNode != nullptr &&
            oldNode->network.prefix == newNode->network.prefix &&
            oldNode->network.length == newNode->network.length)
        {
            oldNode->real = newNode->real;
            oldNode->network.flags = newNode->network.flags;
            return true;
        }
        return false;
    }
    LookupResult nearest(const T ip, const ui32 networkPrefixLength) const {
        std::shared_ptr<NetworkTree<T>::RadixNode> parent = nullptr;
        std::shared_ptr<NetworkTree<T>::RadixNode> current = root;
        ui32 flags = 0;
        while (current != nullptr) {
            if (parent != nullptr && parent->real) {
                flags |= parent->network.flags;
            }
            ui32 prefixLength = std::min(current->network.length, commonPrefixLength(ip, current->network.prefix));
            if (current->network.length <= prefixLength && current->network.length < networkPrefixLength) {
                if (prefixLength + 1 > sizeof(T) * 8) {
                    break;
                }
                ui32 nextBit = getBit(ip, prefixLength + 1);
                if (current->children[nextBit] != nullptr) {
                    parent = current;
                    current = current->children[nextBit];
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        return NetworkTree<T>::LookupResult(parent, current, flags);
    }
    LookupResult findNet(const T ip) const {
        std::shared_ptr<NetworkTree<T>::RadixNode> current(root);
        std::shared_ptr<NetworkTree<T>::RadixNode> lastReal;
        ui32 flags = 0;

        while (current != nullptr && current->network.hasIp(ip)) {
            if (current->real) {
                flags |= current->network.flags;
                lastReal = current;
            }

            ui32 prefixLength = std::min(current->network.length, commonPrefixLength(ip, current->network.prefix));
            if (current->network.length <= prefixLength) {
                if (prefixLength + 1 > sizeof(T) * 8) {
                    break;
                }
                ui32 nextBit = getBit(ip, prefixLength + 1);
                if (current->children[nextBit] != nullptr) {
                    current = current->children[nextBit];
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        return NetworkTree<T>::LookupResult(nullptr, lastReal, flags);
    }
    void split(std::shared_ptr<NetworkTree<T>::RadixNode> parentNode,
               std::shared_ptr<NetworkTree<T>::RadixNode> splitNode,
               std::shared_ptr<NetworkTree<T>::RadixNode> newNode) {
        if (root == nullptr) {
            root = newNode;
            return;
        }
        if (mergeNodes(parentNode, newNode) || mergeNodes(splitNode, newNode)) {
            return;
        }
        ui32 l =
            commonPrefixLength(splitNode->network.prefix, newNode->network.prefix);
        if (l < std::min(splitNode->network.length, newNode->network.length)) {
            T mask = (T(1) << (sizeof(T) * 8 - l)) - 1;
            if (l == 0) {
                mask = T(-1);
            }
            auto glueNode = std::make_shared<NetworkTree<T>::RadixNode>(
                Network<T>(newNode->network.prefix & ~mask, l, 0),
                false);
            rotateNodes(parentNode, splitNode, glueNode);
            ui32 nextBit = getBit(splitNode->network.prefix, l + 1);

            glueNode->children[nextBit] = splitNode;
            glueNode->children[1 - nextBit] = newNode;
        } else {
            if (splitNode->network.length < newNode->network.length) {
                ui32 nextBit =
                    getBit(newNode->network.prefix, splitNode->network.length + 1);
                splitNode->children[nextBit] = newNode;
            } else {
                rotateNodes(parentNode, splitNode, newNode);
                ui32 nextBit =
                    getBit(splitNode->network.prefix, newNode->network.length + 1);
                newNode->children[nextBit] = splitNode;
            }
        }
    }
    inline ui32 commonPrefixLength(T a, T b) const {
        return clz(a ^ b);
    }
    std::string toPartString(const std::shared_ptr<NetworkTree<T>::RadixNode> radixNode, const ui32 level = 0) const {
        std::string resultString = "";
        if (radixNode == nullptr) {
            return resultString;
        }
        for (ui32 i = 0; i < level; ++i) {
            resultString += '|';
        }
        resultString += ("-" + radixNode->network.toString());
        if (!radixNode->real) {
            resultString += "*";
        }
        resultString += "\n";
        resultString += (toPartString(radixNode->children[0], level + 1));
        resultString += (toPartString(radixNode->children[1], level + 1));
        return resultString;
    }

public:
    struct LookupNetwork {
        Network<T> network;
        ui32 flags = 0;
        bool existent = false;
        LookupNetwork(const Network<T> network, const ui32 flags, const bool existent = false)
            : network(network)
            , flags(flags)
            , existent(existent)
        {
        }
        LookupNetwork() = default;
    };

    void add(Network<T> network) {
        auto newNode = std::make_shared<NetworkTree<T>::RadixNode>(network, true);
        auto nearestNodes = nearest(network.prefix, network.length);
        split(nearestNodes.parent, nearestNodes.node, newNode);
    }
    LookupNetwork getNet(const T ip) const {
        auto nearestNodes = findNet(ip);
        ui32 flags = nearestNodes.flags;
        if (nearestNodes.node != nullptr) {
            flags |= nearestNodes.node->network.flags;
            return LookupNetwork(nearestNodes.node->network, flags, true);
        }
        return LookupNetwork(Network<T>(0, 0, 0), 0, false);
    }
    bool isIn(const T ip) const {
        std::shared_ptr<NetworkTree<T>::RadixNode> nearestNode = findNet(ip).node;
        if (nearestNode != nullptr) {
            return nearestNode->network.hasIp(ip);
        }
        return false;
    }

    std::string toString() const {
        return toPartString(this->root);
    }
};

