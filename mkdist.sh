mkdir -p bin/intel64
mkdir -p bin/mic
mkdir -p bin/intel64/platforms
rm -f bin/intel64/speedometer.bin bin/mic/speedometer.bin
cp plotter/plotter monitor/monitor intel64/speedometer.bin bin/intel64
cp scripts/speedometer bin/intel64
cp mic/speedometer.bin bin/mic
cp scripts/speedometer bin/mic
cp libqt/libqxcb.so bin/intel64/platforms
