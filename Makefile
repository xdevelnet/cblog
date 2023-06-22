.PHONY: all fcgi mon demo clean
all:
	@echo Use any of available ways to use this application:
	@echo
	@echo make fcgi
	@echo make mon
	@echo make demo
fcgi:
	cc --std=c99 src/fcgi_version.c -I ../ssb/src/ -O0 -g -o build/cblog_fcgi_debug -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread -lfcgi
	cc --std=c99 src/fcgi_version.c -I ../ssb/src/ -O3 -o build/cblog_fcgi -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread -lfcgi
	strip build/cblog_fcgi
mon:
	cc ../mongoose/mongoose.c src/mon_version.c -I ../mongoose/ -I ../ssb/src/ -Wall -Wextra -O0 -g -o build/cblog_mon_debug -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter
	cc ../mongoose/mongoose.c src/mon_version.c -I ../mongoose/ -I ../ssb/src/ -Wall -Wextra -O3 -o build/cblog_mon -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter
	strip build/cblog_mon
demo:
	cc --std=c99 src/demo.c -O3 -o build/demo -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread
clean:
	rm -f build/*
