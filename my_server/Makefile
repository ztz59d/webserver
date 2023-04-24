# objects =  buf.o epoller.o connection.o http.o lock.o server.o

# server.a : $(objects)
# 	g++ -o server.a $(objects)

# buf.o : buf/buf.h
# 	g++ -c buf/buf.h -o buf.o

# epoller.o : server/epoller.h
# 	g++ -c server/epoller.h -o epoller.o

# connection.o : http/connection.h
# 	g++ -c http/connection.h -o connection.o

# http.o : http/http.h http/http.cpp
# 	g++ -c http/http.h http/http.cpp

# lock.o : lock/lock.h
# 	g++ -c lock/lock.h -o lock.o

# server.o : server/server.h server/server.cpp
# 	g++ -c server/server.h server/server.cpp

objects = server.o http.o

serv : $(objects)
	g++ $(objects) -o serv

http.o : http/http.cpp
	g++ -c http/http.cpp -o http.o

server.o : server/test.cpp
	g++ -c server/test.cpp -o server.o


.PHONY : clean
clean :
	-rm $(objects)