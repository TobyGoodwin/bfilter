depends bfilter

export BFILTER_DB=$(mktemp -u)
echo $BFILTER_DB

../bfilter -b train spam <<EOF
From nobody
Dull: header

Egg and bacon
Egg, sausage and bacon
Egg and Spam

From somebody
Dull: header

Egg, bacon and Spam
Egg, bacon, sausage and Spam
Spam, bacon, sausage and Spam

From anybody
Dull: header

Spam, egg, Spam, Spam, bacon and Spam
Spam, Spam, Spam, egg and Spam
Spam, Spam, Spam, Spam, Spam, Spam, baked beans, Spam, Spam, Spam and Spam
EOF

../bfilter -b train ham <<EOF
From anyone
Dull: header

Lobster Thermidor aux crevettes with a Mornay sauce, garnished with truffle pâté, brandy and a fried egg on top, and Spam.

From someone
Dull: header

Probably pining for the fjords.
EOF

act=$(mktemp)
input=$(echo $target | sed 's,.sh$,.in,')
exp=$(echo $target | sed 's,.sh$,.exp,')
../bfilter classify < $input > $act
../bfilter rename ham real >> $act
../bfilter classify < $input >> $act
if diff -q $exp $act; then
        pass rename
else
        diff -u $exp $act
        fail rename
fi
