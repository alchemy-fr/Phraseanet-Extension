./configure
make
make test
sudo make install

DO NOT 'phpize' before configure !

to test multi-thread queries (experimental), add option "--enable-maintainer-zts" to ./configure
