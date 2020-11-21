# centos 7 compilation info


sudo yum check-update
sudo yum install centos-release-scl
sudo yum install devtoolset-8
scl enable devtoolset-8 bash

yum install boost-devel
yum install glog-devel
yum install double-conversion-devel
yum install snappy-devel
sudo yum install jemalloc-devel
sudo yum install fmt-devel
sudo yum install libsodium-devel
sudo yum install gtest-devel
sudo yum install gmock-devel
sudo yum install gperf
sudo yum install libzstd-devel
sudo yum install xmlto
sudo yum install lzma-devel




## turn on modern GCC

scl enable devtoolset-8 bash

and check that gcc version is more than 8.3.1
gcc --version

now we can compile libraries


## compile from sources code

### BOOST

cd ~/Development/
wget https://dl.bintray.com/boostorg/release/1.74.0/source/boost_1_74_0.tar.gz -O boost.tar.gz
tar -xzvf boost.tar.gz
cd boost_1_74_0/
./bootstrap
./b2 install --with=all

### CMAKE   needed only if cmake 2 version installed. Needed 3.
cd ~/Development/
wget https://github.com/Kitware/CMake/releases/download/v3.19.0/cmake-3.19.0.tar.gz -O cmake.tar.gz
tar -xzvf cmake.tar.gz
cd cmake-3.19.0/
mkdir release
cd release/
cmake ../
make -j12
sudo make install


### fmt
cd ~/Development/
wget https://github.com/fmtlib/fmt/archive/7.1.2.tar.gz -O fmt.tar.gz
tar -xzvf fmt.tar.gz
cd fmt
mkdir release
cd release
cmake ../
make
sudo make install

### FOLLY

sudo yum install epel-release
sudo yum install boost boost-thread boost-devel

cd ~/Development/
wget https://github.com/facebook/folly/archive/v2020.11.16.00.tar.gz -O folly.tar.gz
tar -xzvf folly.tar.gz

cd folly-2020.11.16.00/
mkdir release
cd release
cmake ../
make
sudo make install

### FIZZ

cd ~/Development/
wget https://github.com/facebookincubator/fizz/archive/v2020.11.16.00.tar.gz -O fizz.tar.gz
tar -xzvf fizz.tar.gz
cd fizz-2020.11.16.00/fizz/
mkdir release
cd release
cmake ../
make
sudo make install

### WANGLE
cd ~/Development/
wget https://github.com/facebook/wangle/archive/v2020.11.16.00.tar.gz -O wangle.tar.gz
tar -xzvf wangle.tar.gz

cd wangle-2020.11.16.00/wangle/
mkdir release
cd release
cmake ../
make
sudo make install



### PROXYGEN
cd ~/Development/
wget https://github.com/facebook/proxygen/archive/v2020.11.16.00.tar.gz -O proxygen.tar.gz
tar -xzvf proxygen.tar.gz
cd proxygen-2020.11.16.00/proxygen/
./build.sh
sudo ./install

in /usr/local/lib must be cmake for proxygen and proxygen server

### GeoIP

cd ~/Development/
wget https://github.com/maxmind/geoip-api-c/archive/v1.6.12.tar.gz -O geoip.tar.gz
tar -xzvf geoip.tar.gz
cd geoip-api-c-1.6.12/
./bootstrap
./configure
make -j12


### pqxx

cd ~/Development/
wget https://github.com/jtv/libpqxx/archive/6.4.5.tar.gz -O pqxx.tar.gz
tar -xzvf pqxx.tar.gz
cd libpqxx-6.4.5/



### compilation of program

goes to folder urlfasthash


mkdir release
cd release
cmake ../
make

and watch on compilation error or link troubles.
If there are some linker troubles you can add needed libraries in 

set(LIBS section in CMAkeLists.txt

### create database

use prepare_database.sql for initial tables create


create new url

```
psql -U postgres -d url_proxy -c "INSERT INTO url(url_id, url, created_on, default_url, no_url_failover_url, expired_url_failover_url, out_of_reach_failover_url) VALUES (0, $@'192.99.10.113:12000$@', current_timestamp, $@'http://google.com$@', $@'http://rsdn.ru$@', $@'http://boost.org$@', $@'http://microsoft.com$@');"
```

### run app
cd /home/ivan/Development/urlfasthash/release/
mkdir logs

```
./app --postgres "user=postgres host=localhost port=5432 dbname=url_proxy" --geoip /home/ivan/Development/urlfasthash/release/geopip/GeoIP.dat --alsologtostderr=1 --log_dir=/home/ivan/Development/urlfasthash/release/logs --v=1 --ip 192.99.10.113
```

you can add this parameters to control:
--alsologtostderr=1  - logs to console
--log_dir=/home/ivan/Development/urlfasthash/release/logs
--api_http_port 11000     - port for api calls
--redirect_http_port 12000     - port for client redirections
--ip 192.99.10.113                  - ip listen on
--threads 10                      - how many threads use for server
--clicks_bulk 1000                - how many clicks stored on one transaction commit in separate thread
--v=1                             - verbosity level ... if you want to watch verbose messages
--use_async_commit=True			  - if you need huge count of /api/create calls


if it's ok... you wil see something like this:
I1119 15:19:06.432566 21950 main.cpp:86] api http_port: 11000
I1119 15:19:06.433118 21950 main.cpp:87] redirect http_port: 12000
I1119 15:19:06.433142 21950 main.cpp:88] ip: localhost
I1119 15:19:06.433167 21950 main.cpp:89] threads: 0
Loading tree from db = 4[ms]
Count: 0
Loading domains: 9[ms]
Count: 0

### culr create api

now with curl we can launch this on your local machine ... if 11000 port is opened:

```
curl --header "Content-Type: application/json" \
		  --request POST \
		  --data '{"orig_url":"https://stackoverflow.com/","created_on":"2020-11-17 17:39:49.546162", "expired_on" : "2021-11-17 17:39:49.546162", "sms_uuid":"knockknock", "domain":"192.99.10.113:12000"}' \
		  http://192.99.10.113:11000/api/create
```

### watch clicks table

```
psql -U postgres -d url_proxy -c "select * from clicks"
```