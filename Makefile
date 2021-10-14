.PHONY: all clean
fcgi:
	cc --std=c11 fcgi_version.c -O0 -g -o cblog_fcgi -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread -lfcgi
	cc --std=c11 fcgi_version.c -O3 -o cblog_fcgi_O3 -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread -lfcgi
mon:
	cc ../mongoose/mongoose.c mon_version.c -I ../mongoose/ -Wall -Wextra -O0 -g -o cblog_mon -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter
clean:
	rm -f cblog_fcgi cblog_fcgi_O3 cblog_mon
