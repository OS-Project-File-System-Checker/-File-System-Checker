all: build

build:
	gcc checker.c -o checker
	gcc error_generator.c -o error

clean:
	@echo "Cleaning..."
	rm error
	rm checker

generr: 
	gcc error_generator.c -o error
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


rmerr:
	@echo "Removing error files.."
	rm e1.img
	rm e2.img
	rm e3.img
	rm e4.img
	rm e5.img
	rm e6.img
	rm e7.img
	rm e8.img
	rm e9.img
	rm e10.img
	rm e11.img
	rm e12.img

checkerr:
	gcc checker.c -o checker
	@echo "Checking all err files..."
	@echo ""
	@echo "Errors in file 1: "
	./checker e1.img
	@echo ""

	@echo ""
	@echo "Errors in file 2: "
	./checker e2.img
	@echo ""

	@echo ""
	@echo "Errors in file 3: "
	./checker e3.img
	@echo ""

	@echo ""
	@echo "Errors in file 4: "
	./checker e4.img
	@echo ""

	@echo ""
	@echo "Errors in file 5: "
	./checker e5.img
	@echo ""

	@echo ""
	@echo "Errors in file 6: "
	./checker e6.img
	@echo ""

	@echo ""
	@echo "Errors in file 7: "
	./checker e7.img
	@echo ""

	@echo ""
	@echo "Errors in file 8: "
	./checker e8.img
	@echo ""

	@echo ""
	@echo "Errors in file 9: "
	./checker e9.img
	@echo ""

	@echo ""
	@echo "Errors in file 10: "
	./checker e10.img
	@echo ""

	@echo ""
	@echo "Errors in file 11: "
	./checker e11.img
	@echo ""

	@echo ""
	@echo "Errors in file 12: "
	./checker e12.img
	@echo ""
