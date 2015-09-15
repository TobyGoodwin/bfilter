echo in run.sh

check() {
	r=$(./tester "$1" 2>&1)
	equal "$r" "$2"
}

cp $target main.c
make -C.. test/tester &&
	script_from $target
