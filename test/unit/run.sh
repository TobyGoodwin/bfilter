check() {
	r=$($command "$1")
	equal "$r" "$2"
}

depends() {
    if make -C.. $1; then
        command=./$(basename $1)
    else
        fail "cannot build $1"
        command=:
    fi
}

script_from $target

exit 0


cp $target main.c
if make -C.. test/tester; then
	script_from $target
else
        fail 'cannot build tester'
fi
