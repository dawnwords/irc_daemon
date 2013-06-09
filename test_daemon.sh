cd src/daemon
make
killall -9 srouted
mv ../../test_checkpoint1/srouted ../../
cp srouted ../../test_checkpoint1
make clean
cd ../../test_checkpoint1
ruby test_rdaemon.rb
mv ../srouted .
