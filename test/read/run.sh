depends test/iread
act=$(mktemp)
exp=$(echo $target | sed 's,.in$,.out,')
echo $command $target '>' $act
$command $target > $act
if diff -q $act $exp; then
	pass $target
else
        diff -u $exp $act
	fail $target
fi
rm $act
