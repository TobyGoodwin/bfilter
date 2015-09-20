check() {
	r=$(./tester "$1")
	equal "$r" "$2"
}

cp $target main.c
if make -C.. test/tester; then
	script_from $target
else
        fail 'cannot build tester'
fi
