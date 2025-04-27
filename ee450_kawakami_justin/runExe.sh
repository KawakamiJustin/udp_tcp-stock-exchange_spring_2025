#!/bin/bash

# clear executables
make clean

# Build all executables
make all

# Paths to your C++ executables
EXECUTABLES=("serverM" "serverA" "serverQ" "serverP" "client")

# Minimum number of terminals to keep open
MIN_TERMINALS=3

# Function to check the number of currently open terminals
check_open_terminals() {
    TERMINAL_COUNT=$(pgrep -c gnome-terminal)
    echo $TERMINAL_COUNT
}

# Run the executables in sequence
for exec in "${EXECUTABLES[@]}"
do
    current_terminals=$(check_open_terminals)

    if (( current_terminals < MIN_TERMINALS )); then
        gnome-terminal -- bash -c "./$exec; exec bash"
    else
        echo "Waiting for available terminal before running ./$exec..."
        while (( $(check_open_terminals) >= MIN_TERMINALS )); do
            sleep 1
        done
        gnome-terminal -- bash -c "./$exec; exec bash"
    fi
done