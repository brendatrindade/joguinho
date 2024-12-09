all:
	clear
#	gcc main.c -o exe
#	gcc mouse.c -o exe
#	gcc meta.c proc_grafico.s mouse.c -o exe
	gcc src/labirintoVGA.c src/proc_grafico.s src/mouse.c -o exe -std=c99
	sudo ./exe