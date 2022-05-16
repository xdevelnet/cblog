.PHONY: all fcgi mon demo clean
all:
	@echo Use any of available ways to use this application:
	@echo
	@echo make fcgi
	@echo make mon
	@echo make demo
fcgi:
	cc --std=c99 fcgi_version.c -I ../ssb/src/ -O0 -g -o cblog_fcgi -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread -lfcgi
	cc --std=c99 fcgi_version.c -I ../ssb/src/ -O3 -o cblog_fcgi_O3 -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread -lfcgi
	strip cblog_fcgi_O3
mon:
	cc ../mongoose/mongoose.c mon_version.c -I ../mongoose/ -I ../ssb/src/ -Wall -Wextra -O0 -g -o cblog_mon -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter
	cc ../mongoose/mongoose.c mon_version.c -I ../mongoose/ -I ../ssb/src/ -Wall -Wextra -O3 -o cblog_mon_O3 -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter
	strip cblog_mon_O3
demo:
	cc --std=c99 demo.c -O3 -o demo -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread
clean:
	rm -f cblog_fcgi cblog_fcgi_O3 cblog_mon cblog_mon_O3 demo
