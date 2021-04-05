echo "Starting to build .deb"

#Pack existing source and move to tools dir
tar -zcvf wolkconnect-c_0.0.orig.tar.gz --exclude=build --exclude==tools --exclude=.git --exclude=out/bin \
--exclude=out/CMake* --exclude=out/cmake_install.cmake --exclude=out/Makefile ../../WolkConnect-C
tar -zxvf wolkconnect-c_0.0.orig.tar.gz

#make .deb
cd WolkConnect-C && debuild -us -uc -b -j$(nproc)

#Remove unnecessary
cd ../ && rm -r -f WolkConnect-C && rm *.ddeb *.build *.buildinfo *.changes *.tar.gz