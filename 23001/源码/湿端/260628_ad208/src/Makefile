SRC = beam.c \
      beam_globals.c \
      beam_fpga_io.c \
      beam_network.c \
      beam_config.c \
      beam_control.c \
      beam_sensor.c \
      beam_data.c \
      beam_threads.c
OBJ = app

$(OBJ): $(SRC)
	$(CROSS_COMPILE)gcc $(SRC) -o $(OBJ) -lpthread -lm
	cp app ../bin/

clean:
	rm -f *.o app ../bin/app