#!/bin/bash

BROKER="localhost"
GAME="game1"
BOT_PLAYER="X"

TOPIC_MOVE="ttt/$GAME/move"
TOPIC_STATE="ttt/$GAME/state"
TOPIC_STATUS="ttt/$GAME/status"

choose_move() {
    state=$(mosquitto_sub -h "$BROKER" -t "$TOPIC_STATE" -C 1)

    IFS='|' read -ra cells <<< "$state"

    available=()

    for cell in "${cells[@]}"; do
        if [[ "$cell" =~ ^[1-9]$ ]]; then
            available+=("$cell")
        fi
    done

    count=${#available[@]}

    if [[ "$count" -eq 0 ]]; then
        echo "No moves available"
        return
    fi

    random_index=$((RANDOM % count))
    move="${available[$random_index]}"

    echo "Bot plays: $BOT_PLAYER $move"
    mosquitto_pub -h "$BROKER" -t "$TOPIC_MOVE" -m "$BOT_PLAYER $move"
}

echo "Random Tic-Tac-Toe bot started as player $BOT_PLAYER"
echo "Waiting for TURN $BOT_PLAYER..."

mosquitto_sub -h "$BROKER" -t "$TOPIC_STATUS" | while read -r status; do
    echo "Status: $status"

    if [[ "$status" == "TURN $BOT_PLAYER" ]]; then
        sleep 1
        choose_move
    fi
done