OBJS = prime.o
DEPS = 
OUT = main.out
CPPFLAGS = -Wall -Wextra -std=c++17 -O3 -pedantic-errors -pthread
#CPPFLAGS = -Wall -Wextra -std=c++17 -Og -g -pedantic-errors

$(OUT): $(OBJS)
	g++ $(CPPFLAGS) -o $@ $^

%.o: %.cpp $(DEPS)
	g++ $(CPPFLAGS) -c -o $@ $<

run: $(OUT)
	./main.out 2>&1 >primes.txt |tee speed.txt

clean:
	rm $(OUT) $(OBJS)

