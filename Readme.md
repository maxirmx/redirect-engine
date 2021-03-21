# centos 7 compilation info

```
sudo yum check-update
sudo yum install centos-release-scl
sudo yum install devtoolset-8
sudo yum install glog-devel double-conversion-devel snappy-devel jemalloc-devel libevent-devel
sudo yum install libsodium-devel gtest-devel gmock-devel gperf libzstd-devel xmlto xz-devel bzip2-devel openssl-devel gflags-devel
sudo yum install python3 postresql-devel
```

**turn on GCC 8.x for current session**

scl enable devtoolset-8 bash

**Check that gcc version is higher than 8.3.1**

gcc --version


## CMAKE3   
**This step if required if no cmake or cmake 2.x is installed**
**Check it with cmake --version**

```
cd ~/Development/
wget https://github.com/Kitware/CMake/releases/download/v3.19.0/cmake-3.19.0.tar.gz -O cmake.tar.gz
tar -xzvf cmake.tar.gz
cd cmake-3.19.0/
./bootstrap --prefix=/usr/local
make -j12
sudo make install
```

## BOOST
```
cd ~/Development/
wget https://dl.bintray.com/boostorg/release/1.74.0/source/boost_1_74_0.tar.gz -O boost.tar.gz 
tar -xzvf boost.tar.gz
cd boost_1_74_0/
./bootstrap.sh
./b2 install --without-python 
```

## PROXYGEN
**FMT, FOLLY, FIZZ, WANGLE will be built as PROXIGEN dependencies**
```
cd ~/Development/
wget https://github.com/facebook/proxygen/archive/v2020.11.16.00.tar.gz -O proxygen.tar.gz
tar -xzvf proxygen.tar.gz
cd proxygen-2020.11.16.00/proxygen/
./build.sh
sudo ./install.sh
```
**proxygen installation can be rebased using cmake files at /usr/local/lib**
**otherwise we can just do cp**
```
cp -R ~/Development/proxygen-2020.11.16.00/proxygen/_build/include/* /usr/local/include/
cp -R ~/Development/proxygen-2020.11.16.00/proxygen/_build/lib/* /usr/local/lib/
cp -R ~/Development/proxygen-2020.11.16.00/proxygen/_build/deps/include/* /usr/local/include/
cp -R ~/Development/proxygen-2020.11.16.00/proxygen/_build/deps/lib/* /usr/local/lib/
cp -R ~/Development/proxygen-2020.11.16.00/proxygen/_build/deps/lib64/* /usr/local/lib64/
```

## GeoIP
```
cd ~/Development/
wget https://github.com/maxmind/geoip-api-c/archive/v1.6.12.tar.gz -O geoip.tar.gz
tar -xzvf geoip.tar.gz
cd geoip-api-c-1.6.12/
./bootstrap
./configure
make -j12
sudo make install
```

## pqxx
```
cd ~/Development/
wget https://github.com/jtv/libpqxx/archive/6.4.5.tar.gz -O pqxx.tar.gz
tar -xzvf pqxx.tar.gz
cd libpqxx-6.4.5/
mkdir release
cd release
cmake ../
make
sudo make install
```

## compilation of program
**The repo is in ~/Development/engine**
```
cd ~/Development/engine
mkdir release
cd release
cmake ../
make
```
**And watch on compilation error or link troubles.
If there are some linker troubles you can add needed libraries in** 

**set LIBS section in CmakeLists.txt**

### Initialize database
Run initialization script if you are going to use new database
```
psql -fdb.scripts/db.init.sql
```
or upgrade the legacy version with 
```
psql -fdb.scripts/db.upgrade.x  
```
where x - the number of upgrade to apply

### run app

```
cd ~/Development/engine/release/
mkdir logs
./app --postgres "user=postgres host=localhost port=5432 dbname=url_proxy" --geoip ~/Development/geoipdat/GeoIP.dat --alsologtostderr=1 --log_dir=./logs --v=1 --ip 192.99.10.113 --use_async_commit=True
```

Command line parameters:
```
--alsologtostderr=1  - logs to console
--log_dir=/home/ivan/Development/engine/release/logs
--api_http_port 11000     - port for api calls
--redirect_http_port 12000     - port for client redirections
--ip 192.99.10.113                  - ip listen on
--threads 10                      - how many threads use for server
--clicks_bulk 1000                - how many clicks stored on one transaction commit in separate thread
--v=1                             - verbosity level ... if you want to watch verbose messages
--use_async_commit=True			  - if you need huge count of /api/create calls
```

if it's ok... you wil see something like this:
```
I1119 15:19:06.432566 21950 main.cpp:86] api http_port: 11000
I1119 15:19:06.433118 21950 main.cpp:87] redirect http_port: 12000
I1119 15:19:06.433142 21950 main.cpp:88] ip: localhost
I1119 15:19:06.433167 21950 main.cpp:89] threads: 0
Loading tree from db = 4[ms]
Count: 0
Loading domains: 9[ms]
Count: 0
```

### API for create/update or delete DOMAIN

* this command create or update domain:

```
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "192.99.10.113:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://nyt.com", "no_url_failover_url" : "http://www.washingtonpost.com/", "expired_url_failover_url" : "http://www.latimes.com/", "out_of_reach_failover_url" : "http://www.chicagotribune.com/", "whitelist" : ["RU", "US"]}' \
                  http://192.99.10.113:11000/api/update_domain
```

* this command delete domain and all mapping associated with domain
```
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "192.99.10.113:12000" }' \
                  http://192.99.10.113:11000/api/delete_domain
```

### API for redirect create/update/delete

* create

```
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url":"http://www.dallasnews.com/","created_on":"2020-11-17 17:39:49.546162", "expired_on" : "2021-11-17 17:39:49.546162", "sms_uuid":"827dd855fc1c", "domain":"192.99.10.113:12000", "whitelist":["RU", "US"]}' \
                  http://192.99.10.113:11000/api/create
```

* update

```
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"newUrl" : "http://192.99.10.113:12000/LBItIU", "orig_url" : "http://lamoda.ru", "expired_on" : "2022-11-17 12:00:00", "sms_uuid" : "ywtwy", "whitelist" : ["", "RU", "US"]}' \
                  http://192.99.10.113:11000/api/update_redirect
```

* delete

```
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"newUrl" : "http://localhost:12000/LBItIU" }' \
                  http://192.99.10.113:11000/api/delete_redirect
```

### watch clicks table count

```
psql -U postgres -d url_proxy -c "select * from clicks"
```

