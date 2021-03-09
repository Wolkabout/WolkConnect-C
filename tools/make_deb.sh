echo "Starting to build .deb"
cd ../
debuild -us -uc -b -j$(nproc)
cd ../
mv *.deb *.build *.buildinfo *.changes WolkConnect-C/tools