
set -e
./compile test && ./main
./compile cli && ./main -f fus/cli_test.fus
./compile lang && ./main -f fus/lang_test.fus -d test -e
