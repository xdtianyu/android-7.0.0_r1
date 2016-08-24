#!/bin/sh
# Usage: add-openssl-roots.sh <roots dir> <baseline file>

# Strip all openssl entries
sed -i -e '/openssl/d' "$2"
sed -i -e 's/both/nss/' "$2"

# Re-add them as needed
fingerprints=$(for x in "$1"/*.pem; do \
                   openssl x509 -in "$x" -noout -fingerprint | cut -f2 -d=; \
               done)
for x in $fingerprints; do
	if grep -q "nss $x" "$2"; then
		sed -i -e "s/nss $x/both $x/" "$2"
	fi
	if grep -qE "(both|openssl) $x" "$2"; then
		continue
	fi
	echo "openssl $x" >> "$2"
done

# Re-sort the file
mv "$2" "$2.tmp"
sort "$2.tmp" > "$2"
rm "$2.tmp"
