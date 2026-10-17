#!/bin/sh
set -e

if [ -d ../hare2x ]; then
    pika=..
elif [ -d ../pika/hare2x ]; then
    pika=../pika
elif [ -d ./pika/hare2x ]; then
    pika=./pika
else
    pika="$HOME/pika"
fi

if [ ! -d "$pika" ]; then
    mkdir -p "$pika"
fi
cd "$pika"

for repo in hare2x pika; do
    url="https://github.com/ravengraves/$repo.git"
    if [ -d "$repo" ]; then
        echo "Upgrading $repo"
        git -C "$repo" switch master
        git -C "$repo" pull --ff-only "$url" master
    else
        git clone --branch master "$url" "$repo"
    fi
done

make -C hare2x pika2c
sudo install -d /usr/local/bin
sudo install -m 0755 hare2x/build/pika2c /usr/local/bin/pika2c
PATH="/usr/local/bin:$PATH"
export PATH
cd pika
sh scripts/bootstrap_pikacc.sh
sudo install -m 0755 build/pikacc /usr/local/bin/pikacc
echo "Done"
