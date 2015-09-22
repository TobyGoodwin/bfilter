depends test/uread
act=$(mktemp)
exp=$(echo $target | sed 's,.in,.out,')
$command $target > $act
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
