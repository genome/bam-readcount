#!/bin/bash

cat > scripts/commands.sh <<HERE
#!/bin/bash

dockercmd () {
  docker run --rm -w \$(pwd) -v \$(pwd):\$(pwd) "\$@"
}
HERE

grep 'dockercmd\|paste' README.md | \
  grep -v 'alias\|substitute\|IMAGE' | \
  sed 's/^[ ][ ]*//' \
  >> scripts/commands.sh

chmod +x scripts/commands.sh

