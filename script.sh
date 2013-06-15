cd src/daemon
make
mv srouted ../../finalcheckpoint
make clean
cd ../../finalcheckpoint
ruby script.rb
rm *.conf srouted