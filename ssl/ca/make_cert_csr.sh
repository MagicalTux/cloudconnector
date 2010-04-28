#!/bin/sh
# $1 = csr file

NAME=`certtool --crq-info <"$1" | grep -m 1 Subject | sed -e 's/.*,CN=//;s/,.*//'`

# generate certificate for csr
cat client.tpl | sed -e 's/__NAME__/'"$NAME"'/' >"tmp_tpl.$$"
certtool --generate-certificate --template "tmp_tpl.$$" --load-request "$1" --outfile "ca/$NAME.crt" --load-ca-certificate ca.crt --load-ca-privkey ca.key
rm -f "tmp_tpl.$$"
