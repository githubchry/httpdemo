# httpdemo
openssl+curl+libevent+cjson

## openssl

```
./config --prefix=/home/chry/codes/new-root shared 
make
make install
```


## curl

```
export PKG_CONFIG_PATH=/home/chry/codes/new-root/lib
./configure --enable-static=no --prefix=/home/chry/codes/new-root --with-ssl
make
make install
```

## libevent

```
export LIBS=-I/home/chry/codes/new-root/include/
export LDFLAGS="-L/home/chry/codes/new-root/lib/ -lssl -lcrypto"

./configure --enable-static=no --prefix=/home/chry/codes/new-root 

make
make install
```



## cJSON

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/home/chry/codes/new-root ..

```

