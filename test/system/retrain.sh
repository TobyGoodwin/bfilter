depends bfilter

export BFILTER_DB=$(mktemp -u)
echo $BFILTER_DB

../bfilter train cats <<EOF
Dull: header

abyssinian
birman
tabby
cornish rex
russian blue
EOF

../bfilter train dogs <<EOF
Dull: header

schnauzer
pug
labrador
poodle
EOF

act=$(mktemp)
exp=$(echo $target | sed 's,.sh$,.exp,')
input=$(echo $target | sed 's,.sh$,.in,')

../bfilter classify < ${input}0 > $act
../bfilter train cats < ${input}1
../bfilter classify < ${input}0 >> $act
../bfilter retrain cats dogs < ${input}1
../bfilter classify < ${input}0 >> $act

cat $act
if diff -q $exp $act; then
        pass retrain
else
        diff -u $exp $act
        fail retrain
fi
