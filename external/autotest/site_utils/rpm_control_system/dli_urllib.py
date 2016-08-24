# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import urllib

import dli


class Powerswitch(dli.powerswitch):
    """
    This class will utilize urllib instead of pycurl to get the web page info.
    """


    def geturl(self,url='index.htm') :
        self.contents=''
        path = 'http://%s:%s@%s:80/%s' % (self.userid,self.password,
                                          self.hostname,url)
        web_file = urllib.urlopen(path)
        if web_file.getcode() != 200:
            return None
        self.contents = web_file.read()
        return self.contents
