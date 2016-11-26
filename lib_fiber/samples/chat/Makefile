.PHONEY = all clean cl rebuild rb

all:
	@(cd server; make)
	@(cd client; make)
	@(cd demo; make)

clean cl:
	@(cd server; make clean)
	@(cd client; make clean)
	@(cd demo; make clean)

rebuild rb: cl all
