# centos 7 compilation info


- sudo yum check-update
- sudo yum install centos-release-scl
- sudo yum install devtoolset-8
- yum install boost-devel glog-devel double-conversion-devel snappy-devel jemalloc-devel fmt-devel libsodium-devel gtest-devel gmock-devel gperf libzstd-devel xmlto lzma-devel

## turn on modern GCC

scl enable devtoolset-8 bash

and check that gcc version is more than 8.3.1
gcc --version

**Now we can compile libraries**


## compile from sources code

### BOOST

- cd ~/Development/
- wget https://dl.bintray.com/boostorg/release/1.74.0/source/boost_1_74_0.tar.gz -O boost.tar.gz 
- tar -xzvf boost.tar.gz
- cd boost_1_74_0/
- ./bootstrap
- b2 install --with=all

### CMAKE   needed only if cmake 2 version installed. Needed 3.
- cd ~/Development/
- wget https://github.com/Kitware/CMake/releases/download/v3.19.0/cmake-3.19.0.tar.gz -O cmake.tar.gz
- tar -xzvf cmake.tar.gz
- cd cmake-3.19.0/
- mkdir release
- cd release/
- cmake ../
- make -j12
- sudo make install


### Fmt
- cd ~/Development/
- wget https://github.com/fmtlib/fmt/archive/7.1.2.tar.gz -O fmt.tar.gz
- tar -xzvf fmt.tar.gz
- cd fmt
- mkdir release
- cd release
- cmake ../
- make
- sudo make install

### FOLLY

- sudo yum install epel-release
- sudo yum install boost boost-thread boost-devel

- cd ~/Development/
- wget https://github.com/facebook/folly/archive/v2020.11.16.00.tar.gz -O folly.tar.gz
- tar -xzvf folly.tar.gz

- cd folly-2020.11.16.00/
- mkdir release
- cd release
- cmake ../
- make
- sudo make install

### FIZZ

- cd ~/Development/
- wget https://github.com/facebookincubator/fizz/archive/v2020.11.16.00.tar.gz -O fizz.tar.gz
- tar -xzvf fizz.tar.gz
- cd fizz-2020.11.16.00/fizz/
- mkdir release
- cd release
- cmake ../
- make
- sudo make install

### WANGLE
- cd ~/Development/
- wget https://github.com/facebook/wangle/archive/v2020.11.16.00.tar.gz -O wangle.tar.gz
- tar -xzvf wangle.tar.gz
- cd wangle-2020.11.16.00/wangle/
- mkdir release
- cd release
- cmake ../
- make
- sudo make install

### PROXYGEN
- cd ~/Development/
- wget https://github.com/facebook/proxygen/archive/v2020.11.16.00.tar.gz -O proxygen.tar.gz
- tar -xzvf proxygen.tar.gz
- cd proxygen-2020.11.16.00/proxygen/
- ./build.sh
- sudo ./install

## In /usr/local/lib must be cmake for proxygen and proxygen server##

### GeoIP

- cd ~/Development/
- wget https://github.com/maxmind/geoip-api-c/archive/v1.6.12.tar.gz -O geoip.tar.gz
- tar -xzvf geoip.tar.gz
- cd geoip-api-c-1.6.12/
- ./bootstrap
- ./configure
- make -j12


### pqxx

- cd ~/Development/
- wget https://github.com/jtv/libpqxx/archive/6.4.5.tar.gz -O pqxx.tar.gz
- tar -xzvf pqxx.tar.gz
- cd libpqxx-6.4.5/



### compilation of program

##goes to folder engine##


- mkdir release
- cd release
- cmake ../
- make

**And watch on compilation error or link troubles.
If there are some linker troubles you can add needed libraries in** 

**set LIBS section in CmakeLists.txt**

### create database

**use prepare_database.sql for initial tables create**

### run app
**cd /home/ivan/Development/engine/release/
mkdir logs**

```
./app --postgres "user=postgres host=localhost port=5432 dbname=url_proxy" --geoip /home/ivan/Development/geoipdat/GeoIP.dat --alsologtostderr=1 --log_dir=/home/ivan/Development/engine/release/logs --v=1 --ip 192.99.10.113 --use_async_commit=True
```
`
you can add this parameters to control:
--alsologtostderr=1  - logs to console
--log_dir=/home/ivan/Development/engine/release/logs
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
`

### API for create/update or delete DOMAIN

* this command create or update domain:

```
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "192.99.10.113:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://rsdn.ru", "no_url_failover_url" : "http://google.com", "expired_url_failover_url" : "http://boost.org", "out_of_reach_failover_url" : "http://ori.org", "whitelist" : ["RU", "US"] }' \
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
                  --data '{"orig_url":"http://yandex.ru","created_on":"2020-11-17 17:39:49.546162", "expired_on" : "2021-11-17 17:39:49.546162", "sms_uuid":"knockknock", "domain":"192.99.10.113:12000", "whitelist":["RU", "US"]}' \
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

