TESTSRC := tests/test.c src/celemetry.c

test: $(TESTSRC)
	gcc -g -o test tests/test.c src/celemetry.c

clean:
	rm -f *.o src/*.o test/test