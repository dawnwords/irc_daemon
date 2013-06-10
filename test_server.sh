cd src/server
make
cp sircd ../../test_checkpoint1
make clean
cd ../../test_checkpoint1
ruby checkpoint1.rb
rm sircd
