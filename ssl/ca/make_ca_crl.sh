#!/bin/sh

if [ -f revoked_list.pem ]; then
	echo -e "7\n" | certtool --generate-crl --load-ca-privkey ca.key --load-ca-certificate ca.crt --outfile ca.crl --load-certificate revoked_list.pem
else
	echo -e "7\n" | certtool --generate-crl --load-ca-privkey ca.key --load-ca-certificate ca.crt --outfile ca.crl
fi
