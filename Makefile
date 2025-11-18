TESTSRC := tests/test.c src/celemetry.c
SSDVSRC := tests/ssdv.c src/celemetry.c

all: test ssdvtest

test: $(TESTSRC)
	gcc -g -o test tests/test.c src/celemetry.c

ssdvtest: $(TESTSRC)
	gcc -g -o ssdvtest tests/ssdv.c src/celemetry.c

clean:
	rm -f *.o src/*.o test ssdvtest 