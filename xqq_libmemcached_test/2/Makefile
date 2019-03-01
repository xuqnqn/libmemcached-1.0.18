TARGET = test
SOURCE = test.cc MemCachedClient.h
CFLAG  = -std=c++11 -g3 -O0 -lmemcached
LDFLAG = -L/usr/local/lib/ -lmemcached  
 
TARGET:
	g++ $(CFLAG)  $(SOURCE) $(LDFLAG) -o $(TARGET)
 
.phony:clean
 
clean:
	rm $(TARGET)
 
