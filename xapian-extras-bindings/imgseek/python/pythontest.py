#!/usr/bin/env python

import xapian

# Import xapian.imgseek.
try:
    import xapian.imgseek
except ImportError:
    # Hack to work when uninstalled - normally, this won't be required.
    import imgseek
    xapian.imgseek = imgseek
    del imgseek

db = xapian.inmemory_open()
a = xapian.imgseek.ImgSig()
try:
    a.unserialise('')
except xapian.NetworkError:
    pass
else:
    assert False

b = a.unserialise('\x98\x04\x86\x98\x04\x83\x98\x04\x80\x98\x03\x83\x98\x03\x05\x98\x03\x02\x98\x02\x06\x98\x01\x85\x98\x01\x82\x98\x01\x81\x98\x01\x1b\x98\x01\r\x98\x01\x03\x88\x01\x87\x85\x87\x0b\x87\x05\x87\x01\x07\x03\x07\x04\x07\x06\x07\r\x07\x1b\x076\x07\x80\x07\x83\x18\x01\x05\x18\x01\x80\x18\x01\x83\x18\x01\x86\x18\x01\x8d\x18\x02\x82\x18\x02\x8a\x18\x02\x8e\x08\x03\x18\x03\x03\x18\x03\r\x18\x04\x8b\x08\x05\x18\x06\x86\x98\x03\x0c\x98\x03\x02\x98\x02\x80\x98\x01\x8c\x98\x01\x86\x98\x01\x83\x98\x01\x03\x88\x01\x87\x99\x87\x8c\x87\x86\x87\x17\x87\x0b\x87\x05\x87\x04\x87\x02\x87\x01\x07\x03\x07\x06\x07\x0c\x07\x80\x07\x81\x07\x85\x07\x8b\x07\x97\x18\x01\x01\x18\x01\x02\x18\x01\x80\x18\x01\x82\x18\x01\x85\x18\x01\x8b\x18\x01\x97\x18\x02\x04\x08\x03\x18\x03\x03\x18\x03\x84\x18\x03\x8b\x18\x04\x06\x18\x06\x80\x18\x07\x8b\x88\x04\x98\x03\x80\x98\x02\x80\x88\x02\x98\x01\x8f\x98\x01\x8c\x98\x01\x86\x98\x01\x84\x98\x01\x82\x98\x01\x80\x98\x01\x01\x88\x01\x87\x8c\x87\x86\x87\x85\x87\x83\x87\x82\x87\x81\x87\x80\x87\x17\x87\x0b\x87\n\x87\x05\x87\x04\x87\x02\x87\x01\x07\x03\x07\x06\x07\x07\x07\x0c\x07\x0e\x07\x0f\x18\x01\x02\x18\x01\x04\x18\x01\x8b\x18\x01\x97\x18\x02\x02\x18\x02\x04\x18\x03\x85\x18\x03\x8bfq5X\x13\x15\xb8H\xe5\xe5\x15\xcb*!\xe1`e\xf5\xa7+\t\xfam\xe8')

assert b.score(b) == 100.0