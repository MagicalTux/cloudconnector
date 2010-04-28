#!/bin/sh

# generate CA private key
certtool -p --outfile ca.key --dsa --bits 3072
# Self-sign key to crt
certtool -s --template ca.tpl --load-privkey ca.key --outfile ca.crt
