check() {
	r=$($command "$1")
	equal "$r" "$2"
}

script_from $target
