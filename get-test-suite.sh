#!/bin/sh

if [ -d "test" ]; then
	echo "Test-suite already downloaded"
else
	curl http://www.json.org/JSON_checker/test.zip | tar -x
fi

fuckup() {
	PASS=0
	FAIL=0
	for f in $1; do
		debug/gason-pp "$f" &> /dev/null
		if [ $? == 0 ]; then
			let PASS++
			echo "> $f GOOD"
		else
			let FAIL++
			echo "> $f BAD"
		fi
	done
	echo "=== $PASS/$[PASS+FAIL]"
}

echo "Processing fails...";
fuckup "test/fail*.json"

echo "Processing pass...";
fuckup "test/pass*.json"
