[ ! -d vjson ] && hg clone https://code.google.com/p/vjson/
[ ! -d sajson ] && git clone https://github.com/chadaustin/sajson.git
[ ! -d rapidjson ] && svn checkout http://rapidjson.googlecode.com/svn/trunk/ rapidjson
if [ ! -d shootout ]; then
	mkdir shootout
	cd shootout
	curl -O https://raw.github.com/Newbilius/big_json_import_demo/master/test_data/big.json
	curl https://raw.github.com/mrdoob/three.js/master/examples/models/animated/monster/monster.js > monster.json
	curl -O https://raw.github.com/tcorral/JSONC/master/Benchmark/obj/demo_pack_gzip_compress_with_base64/data/data.js
	echo "console.log(JSON.stringify(obj))" >> data.js
	node < data.js > data.json
	rm data.js
fi
