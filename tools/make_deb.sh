echo "Starting to build .deb"

#Pack existing source and move to tools dir
tar -zcvf wolkconnect-c_0.0.orig.tar.gz --exclude=.git ../../WolkConnect-C
tar -zxvf wolkconnect-c_0.0.orig.tar.gz

#make .deb
cd WolkConnect-C
debuild -us -uc -b -j$(nproc)

#Remove unnecessary
cd ../
rm -r WolkConnect-C && rm *.build *.buildinfo *.changes *.tar.gz