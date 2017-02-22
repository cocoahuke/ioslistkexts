CC=clang
CFLAGS=

build/ioslistkexts:
	mkdir -p build;
	$(CC) $(CFLAGS) src/*.c -o $@

.PHONY:install
install:build/ioslistkexts
	mkdir -p /usr/local/bin
	cp build/ioslistkexts /usr/local/bin/ioslistkexts

.PHONY:uninstall
uninstall:
	rm /usr/local/bin/ioslistkexts

.PHONY:clean
clean:
	rm -rf build
