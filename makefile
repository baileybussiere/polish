polish: src/polish.c src/polishc.c src/lex.h src/fmt-lex.h src/common.h
	gcc -Wall -Wextra src/polish.c -o bin/polish
	gcc -Wall -Wextra src/polishc.c -o bin/polishc

test_lex: test/test_lex.c src/lex.h
	gcc -Wall -Wextra test/test_lex.c -o test/test_lex

debug: src/polish.c src/polishc.c src/lex.h src/fmt-lex.h src/common.h
	gcc -Wall -Wextra --debug -DDEBUG -DSHOWSTACK src/polish.c -o bin/polish
	gcc -Wall -Wextra --debug -DDEBUG -DSHOWSTACK src/polishc.c -o bin/polishc

install:
	cp bin/polishc /usr/local/bin
	cp bin/polish /usr/local/bin
