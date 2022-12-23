FROM ubuntu:18.04
RUN  apt-get update \
  && apt-get -y install software-properties-common \
  && add-apt-repository multiverse \
  && add-apt-repository universe \
  && add-apt-repository restricted
RUN apt-get update \
  && apt-get -y dist-upgrade
RUN apt-get install -y wget vim build-essential gcc gcc-8 g++ g++-8 cmake libpcap-dev htop jnettop screen
RUN apt-get install -y automake pkg-config libtool libtool-bin git tig links
RUN apt-get install -y parallel iptables mahimahi mininet net-tools tcpdump telnet socat
RUN apt-get install -y clang clang-format clang-tidy clang-tools coreutils bash doxygen graphviz netcat-openbsd \
  && rm -rf /var/lib/apt/lists/*
RUN mkdir /data
WORKDIR /data
VOLUME [ "/data" ]