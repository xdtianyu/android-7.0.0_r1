#!/usr/bin/python

import datetime
import getpass
import optparse
import os
import socket


def read_loascertstatus():
  # prodcertstatus --simple_output returns the #of seconds remaining before the
  # cert is expired.
  f = os.popen('prodcertstatus --simple_output | grep LOAS')
  loas_expire = int(f.read().split(':')[1])
  f.close()
  return loas_expire


def main():
  parser = optparse.OptionParser()
  parser.add_option('--expire_within', help='Send email if cert will expire '
                    'within this time window in seconds.',
                    type='int', dest='expire_within', default=24*3600)
  parser.add_option('--to', help='Comma separated Email notification TO '
                    'recipients.', dest='to', type='string', default='')
  parser.add_option('--cc', help='Comma separated Email notification CC '
                    'recipients.', dest='cc', type='string', default='')
  options, _ = parser.parse_args()

  loas_expire = read_loascertstatus()
  host = socket.gethostname()
  if loas_expire < options.expire_within:
    tt = datetime.timedelta(seconds=loas_expire)
    body_text = ('prod access cert (LOAS) for %s will expire within %s on %s.'
                 % (getpass.getuser(), tt, host))
    if not options.to:
      print body_text
    else:
      email_to = ['%s@google.com' % to.strip() for to in options.to.split(',')]

      p = os.popen('/usr/sbin/sendmail -t', 'w')
      p.write('To: %s\n' % ','.join(email_to))
      if options.cc:
        email_cc = ['%s@google.com' % cc.strip()
                    for cc in options.cc.split(',')]
        p.write('Cc: %s\n' % ','.join(email_cc))

      p.write('Subject: Prod access cert (LOAS) for %s will expire soon on %s.'
              '\n' % (getpass.getuser(), host))
      p.write('Content-Type: text/plain')
      p.write('\n')  # blank line separating headers from body
      p.write(body_text)
      p.write('\n')
      return_code = p.close()
      if return_code is not None:
        print 'Sendmail exit status %s' % return_code


if __name__ == '__main__':
  main()
