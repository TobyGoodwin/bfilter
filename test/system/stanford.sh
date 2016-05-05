# This is the worked example from
# http://nlp.stanford.edu/IR-book/html/htmledition/naive-bayes-text-classification-1.html

depends bfilter

export BFILTER_DB=$(mktemp -u)

../bfilter -b train China <<EOF
From nobody
Dull: header

Chinese Beijing Chinese

From somebody
Dull: header

Chinese Chinese Shanghai

From anybody
Dull: header

Chinese Macao

EOF

../bfilter train Japan <<EOF
Dull: header

Tokyo Japan Chinese

EOF

act=$(mktemp)
../bfilter test > $act <<EOF
Dull: header

Chinese Chinese Chinese Tokyo Japan
EOF
if [ $(sed 1q $act) = China ]; then
        pass stanford
else
        cat $act
        fail 'stanford (should classify as China)'
fi
