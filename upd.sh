set -x
cd ArduinoJson
git checkout 6.x
git pull
cd ../hiredis
git checkout master
git pull
make
cd ../limero
git checkout main
git pull
make
cd ../build
make
