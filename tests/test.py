import unittest

import netaddr
from netree import Lookup


USER_FLAG = 0x1


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self.lookup = Lookup()

    def tearDown(self):
        self.lookup.__deleter__()

    def testEasy(self):
        assert self.lookup
        self.lookup.add('85.0.0.0', 8, 0)
        assert self.lookup.is_in('85.0.0.1')
        n = self.lookup.get_net('85.0.0.1')
        assert n == {'flags': 0, 'found': True, 'network': '85.0.0.0/24', 'real_ip': '85.0.0.1'}

        assert not self.lookup.is_in('85.0.1.0')
        n = self.lookup.get_net('85.0.1.0')
        assert n == {'flags': 0, 'found': False, 'real_ip': '85.0.1.0'}

    def testFlags(self):
        self.lookup.add('85.0.0.0', 8, 1)
        assert self.lookup.is_in('85.0.0.1')
        n = self.lookup.get_net('85.0.0.1')
        assert n == {
            'flags': 1,
            'found': True,
            'network': '85.0.0.0/24',
            'real_ip': '85.0.0.1',
        }

        assert not self.lookup.is_in('85.0.1.0')
        n = self.lookup.get_net('85.0.1.0')
        assert n == {'flags': 0, 'found': False, 'real_ip': '85.0.1.0'}

    def testNotSoEasy1(self):
        self.lookup.add('85.0.0.0', 16, 0)
        self.lookup.add('85.0.0.0', 8, 1)
        assert self.lookup.get_net('85.0.0.1') == {
            'flags': 1,
            'found': True,
            'network': '85.0.0.0/24',
            'real_ip': '85.0.0.1',
        }
        assert self.lookup.get_net('85.0.1.0') == {
            'flags': 0,
            'found': True,
            'network': '85.0.0.0/16',
            'real_ip': '85.0.1.0',
        }

    def testNotSoEasy2(self):
        self.lookup.add('85.0.0.0', 8, 1)
        self.lookup.add('85.0.0.0', 16, 0)
        assert self.lookup.get_net('85.0.0.1') == {
            'flags': 1,
            'found': True,
            'network': '85.0.0.0/24',
            'real_ip': '85.0.0.1',
        }
        assert self.lookup.get_net('85.0.1.0') == {
            'flags': 0,
            'found': True,
            'network': '85.0.0.0/16',
            'real_ip': '85.0.1.0',
        }

    def testNotSoEasy3(self):
        self.lookup.add('85.0.0.0', 16, 0)
        self.lookup.add('85.0.0.0', 8, 1)
        assert self.lookup.get_net('85.0.0.1') == {
            'flags': 1,
            'found': True,
            'network': '85.0.0.0/24',
            'real_ip': '85.0.0.1',
        }
        assert self.lookup.get_net('85.0.1.0') == {
            'flags': 0,
            'found': True,
            'network': '85.0.0.0/16',
            'real_ip': '85.0.1.0',
        }

    def testNotSoEasy4(self):
        self.lookup.add('85.0.0.0', 8, 0)
        self.lookup.add('85.0.1.0', 8, 1)
        self.lookup.add('85.0.0.0', 9, 2)
        assert self.lookup.get_net('85.0.0.1') == {
            'flags': 2,
            'found': True,
            'network': '85.0.0.0/24',
            'real_ip': '85.0.0.1',
        }
        assert self.lookup.get_net('85.0.1.1') == {
            'flags': 3,
            'found': True,
            'network': '85.0.1.0/24',
            'real_ip': '85.0.1.1',
        }
        assert self.lookup.get_net('85.0.2.1') == {
            'flags': 0,
            'found': False,
            'real_ip': '85.0.2.1',
        }
        assert str(self.lookup) == 'IPv4 Tree:\n-85.0.0.0/23\n|-85.0.0.0/24\n|-85.0.1.0/24\n\nIPv6 Tree:\n'

    def testIpV6(self):
        data = [
            {'low': '8ddd:312:b012:1000::', 'high': '8ddd:312:b012:1fff:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1000::', 'high': '8ddd:312:b012:1000:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1001::', 'high': '8ddd:312:b012:1001:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1002::', 'high': '8ddd:312:b012:1002:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1003::', 'high': '8ddd:312:b012:1003:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1004::', 'high': '8ddd:312:b012:1004:ffff:ffff:ffff:ffff', 'flags': {'user': True}},
            {'low': '8ddd:312:b012:1005::', 'high': '8ddd:312:b012:1005:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1006::', 'high': '8ddd:312:b012:1006:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1007::', 'high': '8ddd:312:b012:1007:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1008::', 'high': '8ddd:312:b012:1008:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1009::', 'high': '8ddd:312:b012:1009:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:100a::', 'high': '8ddd:312:b012:100a:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:100b::', 'high': '8ddd:312:b012:100b:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:100c::', 'high': '8ddd:312:b012:100c:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:100d::', 'high': '8ddd:312:b012:100d:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:100e::', 'high': '8ddd:312:b012:100e:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:100f::', 'high': '8ddd:312:b012:100f:ffff:ffff:ffff:ffff', 'flags': {}},
            {'low': '8ddd:312:b012:1004:0001::', 'high': '8ddd:312:b012:1004:000f:ffff:ffff:ffff', 'flags': {}},
        ]

        for line in data:
            low = netaddr.IPAddress(line['low'])
            high = netaddr.IPAddress(line['high'])

            is_user = USER_FLAG if line.get('flags', {}).get('user', False) else 0

            mask = int(low) ^ int(high)
            length = mask.bit_length()

            self.lookup.add(low, length, is_user)

        assert self.lookup.get_net('8ddd:312:b012:1004::1') == {
            'flags': 1,
            'found': True,
            'network': '8ddd:312:b012:1004:1::/76',
            'real_ip': '8ddd:312:b012:1004::1',
        }
        assert self.lookup.get_net('8ddd:312:b012:1004:0011::1') == {
            'flags': 1,
            'found': True,
            'network': '8ddd:312:b012:1004::/64',
            'real_ip': '8ddd:312:b012:1004:0011::1',
        }

    def testNotSoEasy5(self):
        self.lookup.add('85.0.0.0', 8, 1)
        self.lookup.add('85.0.0.0', 16, 0)
        self.lookup.add('0.0.0.0', 32, 0)
        self.lookup.add('0.0.0.0', 32, 2)
        self.lookup.add('85.0.0.5', 0, 4)
        self.lookup.add('85.0.0.5', 0, 0)
        self.lookup.add('85.0.0.6', 0, 0)
        self.lookup.add('85.0.0.7', 0, 0)
        assert self.lookup.get_net('85.0.0.1') == {
            'flags': 3,
            'found': True,
            'network': '85.0.0.0/24',
            'real_ip': '85.0.0.1',
        }
        assert self.lookup.get_net('85.0.0.5') == {
            'flags': 3,
            'found': True,
            'network': '85.0.0.5/32',
            'real_ip': '85.0.0.5',
        }
