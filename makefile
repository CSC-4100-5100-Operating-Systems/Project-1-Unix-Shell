# build with: make
# run with: make run
# clean with: make clean

wish: wish.c batch.c
	gcc -o wish wish.c batch.c

clean:
	rm -f wish
