CC = gcc
LD = ld

z64audio: z64snd.c
	@$(CC) -o $@ $< -lm -g -Og -Wall -Wno-unused-variable
