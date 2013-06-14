cd src/daemon
make
mv srouted ../../my_test
make clean
cd ../../my_test
ruby spawn_daemon.rb
