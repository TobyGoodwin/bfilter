check() {
        echo $command "$1"
	r=$($command "$1")
	equal "$r" "$2"
}

mcheck() {
        exp="$1"
        shift
        echo $command "$@"
	r=$($command "$@")
	equal "$r" "$exp"
}

script_from $target
