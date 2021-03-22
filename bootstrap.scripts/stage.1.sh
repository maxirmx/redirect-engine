sudo yum -y update                          \
&&   sudo yum -y install epel-release            \
&&   sudo yum -y install https://packages.endpoint.com/rhel/7/os/x86_64/endpoint-repo-1.7-1.x86_64.rpm \
&&   sudo yum -y install centos-release-scl      \
&&   sudo yum -y remove  git                     \
&&   sudo yum -y install devtoolset-8            \
                    double-conversion-devel \
                    jemalloc-devel          \
                    glog-devel              \
                    gflags-devel            \
                    snappy-devel            \
                    libevent-devel          \
                    libsodium-devel         \
                    gperf                   \
                    libzstd-devel           \
                    xmlto                   \
                    xz-devel                \
                    bzip2-devel             \
                    openssl-devel           \
                    python3                 \
                    wget                    \
                    git                     ; sudo yum clean all  
