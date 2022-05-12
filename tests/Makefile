.PHONY: all clean
all:
	cc --std=c99 test_layer_fileno.c -O0 -g -o test_layer_fileno -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread
	cc --std=c99 test_layer_fileno.c -O3 -o test_layer_fileno_O3 -Wall -Wextra -Wno-unused-result -Wno-misleading-indentation -Wno-unused-parameter -lpthread
clean:
	rm -f test_layer_fileno test_layer_fileno_O3
