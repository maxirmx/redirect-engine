export V_PROXIGEN=2020.11.16.00

cd ~/bootstrap \
&& wget https://github.com/facebook/proxygen/archive/v${V_PROXIGEN}.tar.gz -nv -O proxygen.tar.gz \
&& tar -xzf proxygen.tar.gz \
&& cd ~/bootstrap/proxygen-${V_PROXIGEN}/proxygen/ \
&& sed s/\-DCMAKE_INSTALL_PREFIX=\"\$DEPS_DIR\"/\-DCMAKE_INSTALL_PREFIX=\"\$PREFIX\"/ < build.sh > b.sh \
&& chmod +x b.sh \
&& ./b.sh -j 4 --prefix /usr/local && ./install.sh 
