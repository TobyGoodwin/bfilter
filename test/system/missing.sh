depends bfilter
BFILTER_DB=$(mktemp -u)
act=$(mktemp)
target=system/missing.in
exp=$(echo $target | sed 's,.in$,.out,')
../bfilter classify < $target > $act
if diff -q $exp $act; then
        pass missing database
else
        diff -u $exp $act
        fail missing database
fi
