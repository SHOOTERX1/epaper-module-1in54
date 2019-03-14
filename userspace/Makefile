build: epd

objs:= epaper.o epaper_core.o epaper_paint.o\
font8.o font12.o font16.o font20.o font24.o\
background.o

epd: $(objs)
	gcc $(objs) -o epd

clean:
	rm *.o epd