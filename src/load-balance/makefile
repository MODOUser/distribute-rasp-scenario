#objects: client-test.o 
#
#client-test: $(objects)
#	g++ -o `pkg-config --libs --cflags libzmq` client-test $(objects) -pthread
#
#client-test.o : client-test.cc lbworker.h lbhelper.h
#
#clean: -rm -f *.o

client-test : client-test.o 
	g++ -o client-test client-test.o `pkg-config --libs --cflags libzmq` -pthread

client-test.o : client-test.cc lbworker.h
	g++ -c  client-test.cc

clean : 
	rm client-test.o



#client : client.o 
#	g++ -o client client.o `pkg-config --libs --cflags libzmq`
#
#client.o : client.cc
#	g++ -c client.cc
#
#clean : 
#	rm client.o
