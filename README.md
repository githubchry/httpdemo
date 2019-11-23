# httpdemo

openssl+curl+libevent+cjson

## openssl

```shell
./config --prefix=/home/chry/codes/new-root shared 
make
make install
```

## curl

```shell
export PKG_CONFIG_PATH=/home/chry/codes/new-root/lib
./configure --enable-static=no --prefix=/home/chry/codes/new-root --with-ssl
make
make install
```
<https://www.cnblogs.com/moodlxs/archive/2012/10/15/2724318.html>

## libevent

```shell
export LIBS=-I/home/chry/codes/new-root/include/
export LDFLAGS="-L/home/chry/codes/new-root/lib/ -lssl -lcrypto"

./configure --enable-static=no --prefix=/home/chry/codes/new-root 

make
make install
```

## cJSON

```shell
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/home/chry/codes/new-root ..

```
