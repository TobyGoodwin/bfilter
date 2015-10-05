depends test/upass
act=$(mktemp)
echo $command $target '>' $act
$command $target > $act
if diff -q $act $target; then
	pass $target
else
	echo expected:
	cat $target
	echo actual:
	cat $act
	fail $target
fi
rm $act
