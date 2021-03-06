#!/usr/bin/python

import sys
import streamer
import packetchannel
import unittest

class TestComm(unittest.TestCase):

    def setUp(self):
        self.USB_PORT_MY     = 8887
        self.USB_PORT_OTHER  = 8888
        #self.USB_PORT_MY     = 5554
        #self.USB_PORT_OTHER  = 5555
        self.TEST_PORT_MY    = 6665
        self.TEST_PORT_OTHER = 6666
        
        self.usb_chan = packetchannel.UDPChannel('localhost', self.USB_PORT_MY,
                                                 'localhost', self.USB_PORT_OTHER)
        self.tst_chan = packetchannel.UDPChannel('localhost', self.TEST_PORT_MY,
                                                 'localhost', self.TEST_PORT_OTHER)
                                                
        npackets = 10
        self.usb_strm = streamer.Streamer(self.usb_chan, npackets, npackets, 5, 0.3, 1442)
        self.tst_strm = streamer.Streamer(self.tst_chan, npackets, npackets, 5, 0.3, 1442)
        
    def testCommunication(self):
        self.usb_strm.start()
        self.tst_strm.start()

        usb_out, usb_in = self.usb_strm.join()
        tst_out, tst_in = self.tst_strm.join()
        
        self.assertListsEqual(tst_out, usb_in)
        self.assertListsEqual(usb_out, tst_in)

    def assertListsEqual(self, l1, l2):
        self.assertEquals(len(l1), len(l2))
        for i, e1, e2 in zip(xrange(len(l1)), l1, l2):
            self.assertEquals(e1, e2, "elem %d: %s != %s" % (i, repr(e1), repr(e2)))

unittest.main()
