#!/usr/bin/env python

import cgi,os
import cgitb; cgitb.enable()  # for troubleshooting

print "Content-type: text/html"
print

print """
<html>

<head><title>Sample CGI Script</title></head>

<body>

"""
print os.environ.get("QUERY_STRING", "No Query String in url")

cgi.print_environ()

print """

  <h3> Sample CGI Script </h3>
"""

form = cgi.FieldStorage()
message = form.getvalue("message", "(no message)")

print """

  <p>Previous message: %s</p>

  <p>form

  <form method="post" action="shop.cgi">
    <p>message: <input type="text" name="message"/></p>
  </form>

</body>

</html>
""" % cgi.escape(message)