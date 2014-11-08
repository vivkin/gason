set -x
[ ! -d rapidjson ] && git clone https://github.com/miloyip/rapidjson
[ ! -f canada.json ] && curl -fLO https://raw.githubusercontent.com/mloskot/json_benchmark/master/data/canada.json
[ ! -f big.json ] && curl -fLO https://raw.githubusercontent.com/Newbilius/big_json_import_demo/master/test_data/big.json
[ ! -f citm_catalog.json ] && curl -fLO https://raw.githubusercontent.com/RichardHightower/json-parsers-benchmark/master/data/citm_catalog.json
[ ! -f marine.json ] && curl -fLo marine.json https://raw.githubusercontent.com/mrdoob/three.js/master/examples/models/skinned/marine/marine.js
