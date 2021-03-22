export V_PQXX=6.4.5
export V_GEOIP=1.6.12

# sudo yum install https://download.postgresql.org/pub/repos/yum/reporpms/EL-7-x86_64/pgdg-redhat-repo-latest.noarch.rpm \
sudo yum -y install postgresql-devel \
&& source scl_source enable devtoolset-8 \
&& cd ~/bootstrap \
&& wget https://github.com/jtv/libpqxx/archive/${V_PQXX}.tar.gz -O pqxx.tar.gz \
&& tar -xzvf pqxx.tar.gz && cd libpqxx-${V_PQXX}/ && mkdir -p release && cd release \
&& cmake ../ && make install \
&& cd ~/bootstrap \
&& wget https://github.com/maxmind/geoip-api-c/archive/v${V_GEOIP}.tar.gz -O geoip.tar.gz \
&& tar -xzvf geoip.tar.gz && cd geoip-api-c-${V_GEOIP}/ \
&& ./bootstrap && ./configure && make -j4 && make install \
&& mkdir -p ~/Development/geoipdat \
&& cp ~/bootstrap/geoip-api-c-${V_GEOIP}/data/GeoIP.dat ~/Development/geoipdat/ \
&& mkdir -p ~Development/engine/release/logs

