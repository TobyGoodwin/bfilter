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

testdb() {
        db="$1"
        sqlite3 $db <<EOF
CREATE TABLE class (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  docs INTEGER NOT NULL,
  terms INTEGER NOT NULL,
   UNIQUE (name) );
CREATE TABLE term (
  id INTEGER PRIMARY KEY,
  term TEXT NOT NULL UNIQUE );
CREATE TABLE count (
  class INTEGER NOT NULL,
  term INTEGER NOT NULL,
  count INTEGER,
    PRIMARY KEY (class, term),
    FOREIGN KEY (class) REFERENCES class (id),
    FOREIGN KEY (term) REFERENCES term (id));
CREATE TABLE version (
  version INTEGER NOT NULL );
INSERT INTO version (version) VALUES (3);
INSERT INTO term (term) VALUES('spamword');
INSERT INTO term (term) VALUES('hamword');
EOF
}

script_from $target
