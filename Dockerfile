FROM ubuntu:latest
RUN apt-get update && apt-get install -y g++ libboost-all-dev
WORKDIR /app
COPY . /app
RUN g++ -std=c++11 -o server server.cpp -lboost_system -lpthread
CMD ./server

