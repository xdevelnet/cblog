.PHONY: all fcgi mon clean
all:
	@echo Use any of available ways to use this application: make fcgi or make mon
fcgi:
	cc --std=gnu99 fcgi_version.c -I ../ssb/src/ -O0 -g -o cblog_fcgi -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread -lfcgi
	cc --std=gnu99 fcgi_version.c -I ../ssb/src/ -O3 -o cblog_fcgi_O3 -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread -lfcgi
mon:
	cc ../mongoose/mongoose.c mon_version.c -I ../mongoose/ -I ../ssb/src/ -Wall -Wextra -O0 -g -o cblog_mon -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter
clean:
	rm -f cblog_fcgi cblog_fcgi_O3 cblog_mon
