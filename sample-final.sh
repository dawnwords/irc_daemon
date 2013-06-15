cd src/daemon
make
mv srouted ../../finalcheckpoint
make clean
cd ../../finalcheckpoint
ruby sample_final_test.rb
rm *.conf srouted