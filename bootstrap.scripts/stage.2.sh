export V_CMAKE=3.19.0
export V_BOOST=1.74.0
export V_BOOST_=1_74_0

cd ~/bootstrap \
&& wget https://github.com/Kitware/CMake/releases/download/v${V_CMAKE}/cmake-${V_CMAKE}.tar.gz -nv -O cmake.tar.gz \
&& tar -xzf cmake.tar.gz && cd ~/bootstrap/cmake-${V_CMAKE} \
&& ./bootstrap --prefix=/usr/local && make -j8 install \
&& cd ~/bootstrap \
&& wget https://dl.bintray.com/boostorg/release/${V_BOOST}/source/boost_${V_BOOST_}.tar.gz -nv -O boost.tar.gz \
&& tar -xzf boost.tar.gz && cd ~/bootstrap/boost_${V_BOOST_} \
&& ./bootstrap.sh  && ./b2 -j8 -d1 --without-python --prefix=/usr/local install \
&& cd ~/bootstrap 
#&& rm -rf /bootstrap/boost_${V_BOOST_}
