all: replica client web iniciar

iniciar.o: iniciar.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c iniciar.c -o iniciar.o

iniciar: iniciar.o xarxa.o
	gcc iniciar.o xarxa.o -o iniciar

replica.o: replica.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c replica.c -o replica.o

client.o: client.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c client.c -o client.o

web.o: web.c comu.h websocket.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c web.c -o web.o

xarxa.o: xarxa.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c xarxa.c -o xarxa.o

replica: replica.o xarxa.o
	gcc replica.o xarxa.o -o replica

client: client.o xarxa.o
	gcc client.o xarxa.o -o client

websocket.o: websocket.c websocket.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c websocket.c -o websocket.o

web: web.o xarxa.o websocket.o
	gcc web.o xarxa.o websocket.o -o web

clean:
	rm -f websocket.o replica.o client.o web.o xarxa.o iniciar.o replica client web iniciar
