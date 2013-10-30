#!/bin/sh

if [ -d "test" ]; then
	echo "Test-suite already downloaded"
else
	curl -sSO http://www.json.org/JSON_checker/test.zip && unzip test.zip 2> /dev/null && rm test.zip
fi

count_pass_fail() {
	PASS=0
	FAIL=0
	for f in $1; do
		debug/gason-pp "$f" > /dev/null
		if [ "$?" = 0 ]; then
			PASS=$((PASS=PASS+1))
		else
			FAIL=$((FAIL=FAIL+1))
		fi
	done
	echo "=== $PASS/$((PASS+FAIL))"
}

echo "Processing fails...";
count_pass_fail "test/fail*.json"

echo "Processing pass...";
count_pass_fail "test/pass*.json"
