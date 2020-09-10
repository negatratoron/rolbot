while :; do
    pkill -9 rolbot
    savelog -n1 nohup.out
    ./rolbot
done
