FROM ubuntu:latest
RUN apt-get update && apt-get install -y g++ libboost-all-dev cmake
WORKDIR /app
COPY . /app
RUN cmake .
RUN cmake --build .
CMD ./build/debug/net_olc_simpleServer 

