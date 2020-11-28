all: build

build:
	gcc checker.c -o checker
	gcc error.c -o error

clean:
	@echo "Cleaning..."
	rm error
	rm checker
