# coding: utf-8
import netaddr
import six

from libc.stdint cimport int32_t, uint32_t
from libcpp cimport bool as bool_t
from libcpp.string cimport string as string_t


NOT_NETWORK = 4294967295


cdef extern from "netree.h" nogil:
    # https://stackoverflow.com/questions/27582001/how-to-use-128-bit-integers-in-cython
    ctypedef unsigned long long uint128_t

    cdef cppclass Network[T]:
        T prefix;
        int32_t length
        uint32_t flags
        Network(T prefix, int32_t length, uint32_t flags)

    cdef cppclass NetworkTree[T]:
        cppclass LookupNetwork:
            Network[T] network
            uint32_t flags
            bool_t existent
        void add(Network[T] network)
        LookupNetwork getNet(T ip)
        bool_t isIn(T ip)
        string_t toString()

cdef class Lookup:
    cdef NetworkTree[uint32_t]* v4tree
    cdef NetworkTree[uint128_t]* v6tree

    def __cinit__(self):
        self.v4tree = new NetworkTree[uint32_t]()
        self.v6tree = new NetworkTree[uint128_t]()

    def _get_net_v4(self, ip):
        r = self.v4tree.getNet(int(ip))
        return {
            'network': str(netaddr.IPAddress(r.network.prefix)) + '/' + str(r.network.length),
            'flags': r.flags,
            'length': r.network.length,
            'existent': r.existent,
        }

    def _get_net_v6(self, ip):
        r = self.v6tree.getNet(int(ip))
        return {
            'network': str(netaddr.IPAddress(r.network.prefix)) + '/' + str(r.network.length),
            'flags': r.flags,
            'length': r.network.length,
            'existent': r.existent,
        }

    def _cast_ip(self, ip):
        if isinstance(ip, six.string_types):
            ip = netaddr.IPAddress(ip)
        return ip

    def add(self, ip, length, flags):
        ip = self._cast_ip(ip)
        if ip.version == 6:
            self.v6tree.add(Network[uint128_t](int(ip), 128 - length, flags))
        else:
            self.v4tree.add(Network[uint32_t](int(ip), 32 - length, flags))

    def get_net(self, ip):
        real_ip = self._cast_ip(ip)

        if real_ip.version == 6:
            r = self._get_net_v6(real_ip)
        else:
            r = self._get_net_v4(real_ip)

        if not r['existent']:
            return {
                'real_ip': ip,
                'flags': 0,
                'found': False,
            }

        return {
            'real_ip': ip,
            'network': r['network'],
            'flags': r['flags'],
            'found': True,
        }

    def is_in(self, ip):
        ip = self._cast_ip(ip)
        if ip.version == 6:
            return self.v6tree.isIn(int(ip))
        else:
            return self.v4tree.isIn(int(ip))

    def __dealloc__(self):
        self.__deleter__()

    def __deleter__(self):
        del self.v4tree
        self.v4tree = NULL
        del self.v6tree
        self.v6tree = NULL


    def __str__(self):
        return 'IPv4 Tree:\n' + self.v4tree.toString() + '\nIPv6 Tree:\n' + self.v6tree.toString()
