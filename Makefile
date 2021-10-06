.PHONY: all clean
all:
	cc --std=c11 main.c -O0 -g -o cblog -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -Werror -lpthread -lfcgi
	cc --std=c11 main.c -O3 -o cblog_O3 -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -Werror -lpthread -lfcgi
clean:
	rm -f cblog cblog_O3
