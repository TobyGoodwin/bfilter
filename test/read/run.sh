check() {
	r=$(./tester "$1" 2>&1)
	equal "$r" "$2"
}

make -C.. test/readtester
act=$(mktemp)
exp=$(echo $target | sed 's,.in,.out,')
./readtester $target > $act
if diff -q $act $exp; then
	pass $target
else
	echo expected:
	cat $exp
	echo actual:
	cat $act
	fail $target
fi
rm $act
