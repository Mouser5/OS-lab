OUTPUT="dir_history"
SRC="daemon.cpp"

g++ main.cpp Daemon.cpp -o my_daemon -std=c++17 -Wall -Werror

rm -f *.o