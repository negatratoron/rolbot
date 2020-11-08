while :; do
    pkill -9 rolbot
    savelog -n1 nohup.out
    LD_LIBRARY_PATH=$HOME/local/lib ./rolbot
done
