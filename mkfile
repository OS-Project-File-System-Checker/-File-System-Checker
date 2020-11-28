all: build

build:
	gcc checker.c -o checker
	gcc error.c -o error

clean:
	@echo "Cleaning..."
	rm error
	rm checker

generr: 
	gcc error.c -o error
	@echo "Generating error files..."
	cp fs.img e1.img
	./error 1 e1.img
	cp fs.img e2.img
	./error 2 e2.img
	cp fs.img e3.img
	./error 3 e3.img
	cp fs.img e4.img
	./error 4 e4.img
	cp fs.img e5.img
	./error 5 e5.img
	cp fs.img e6.img
	./error 6 e6.img
	cp fs.img e7.img
	./error 7 e7.img
	cp fs.img e8.img
	./error 8 e8.img
	cp fs.img e9.img
	./error 9 e9.img
	cp fs.img e10.img
	./error 10 e10.img
	cp fs.img e11.img
	./error 11 e11.img
	cp fs.img e12.img
	./error 12 e12.img
