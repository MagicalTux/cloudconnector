#!/bin/sh
NAME=$1
if [ x$NAME = x ]; then
	NAME=`hostname | tr A-Z a-z`
fi

# make client private key
certtool -p --outfile ssl.key --bits 4096
cat ssl.tpl | sed -e 's/__NAME__/'"$NAME"'/' >"tmp_tpl.$$"
certtool -q --template "tmp_tpl.$$" --load-privkey ssl.key --outfile ssl.csr
rm -f "tmp_tpl.$$"
