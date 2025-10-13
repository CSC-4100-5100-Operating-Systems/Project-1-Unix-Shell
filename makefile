# build with: make
# run with: make run
# clean with: make clean

wish:
	gcc -o wish wish.c batch.c parallel.c commands.c wishCwdPrompt.c linenoise.c

clean:
	rm -f wish
