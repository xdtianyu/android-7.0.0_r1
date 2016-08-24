#!/bin/bash

chmod -R o+r /usr/local/autotest
find /usr/local/autotest/ -type d | xargs chmod o+x
chmod o+x /usr/local/autotest/tko/*.cgi
