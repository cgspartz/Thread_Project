all: project2Spartz.o encrypt-module.o 
	gcc -o encrypt352 project2Spartz.o encrypt-module.o  -lpthread

debug: project2.o encrypt-module.o 
	gcc -o encrypt352 -g project2Spartz.o encrypt-module.o  -g -lpthread

test: project2Spartz.o encrypt-module-reproducible.o 
	gcc -o encrypt352 -g project2Spartz.o encrypt-module-reproducible.o  -g -lpthread

project2Spartz.o: project2Spartz.c encrypt-module.h
	gcc -c project2Spartz.c

encrypt-module.o: encrypt-module.c encrypt-module.h
	gcc -c encrypt-module.c

encrypt-module-reproducible.o: encrypt-module-reproducible.c encrypt-module.h
	gcc -c encrypt-module-reproducible.c

clean:
	rm encrypt352 project2Spartz.o encrypt-module.o 