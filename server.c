#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mosquitto.h>

#define MQTT_HOST "localhost"
#define MQTT_PORT 1883

#define TOPIC_MOVE   "ttt/game1/move"
#define TOPIC_STATE  "ttt/game1/state"
#define TOPIC_STATUS "ttt/game1/status"
#define TOPIC_RESET  "ttt/game1/reset"

char board[9] = {'1','2','3','4','5','6','7','8','9'};
char turn = 'X';
bool game_over = false;
FILE *logfile;

void log_msg(const char *type, const char *msg) {
    logfile = fopen("tic_tac_toe_game.log", "a");
    if (logfile) {
        fprintf(logfile, "%s: %s\n", type, msg);
        fclose(logfile);
    }
}

void publish_status(struct mosquitto *mosq, const char *status) {
    mosquitto_publish(mosq, NULL, TOPIC_STATUS, strlen(status), status, 0, true);
    printf("STATUS: %s\n", status);
    log_msg("STATUS", status);
}

void publish_state(struct mosquitto *mosq) {
    char state[32];

    snprintf(state, sizeof(state),
             "%c|%c|%c|%c|%c|%c|%c|%c|%c",
             board[0], board[1], board[2],
             board[3], board[4], board[5],
             board[6], board[7], board[8]);

    mosquitto_publish(mosq, NULL, TOPIC_STATE, strlen(state), state, 0, true);
    printf("STATE: %s\n", state);
    log_msg("STATE", state);
}

void reset_game(struct mosquitto *mosq) {
    for (int i = 0; i < 9; i++) {
        board[i] = '1' + i;
    }

    turn = 'X';
    game_over = false;

    publish_state(mosq);
    publish_status(mosq, "TURN X");
}

char check_winner() {
    int wins[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8},
        {0,3,6}, {1,4,7}, {2,5,8},
        {0,4,8}, {2,4,6}
    };

    for (int i = 0; i < 8; i++) {
        int a = wins[i][0];
        int b = wins[i][1];
        int c = wins[i][2];

        if (board[a] == board[b] && board[b] == board[c]) {
            if (board[a] == 'X' || board[a] == 'O') {
                return board[a];
            }
        }
    }

    return '\0';
}

bool check_draw() {
    for (int i = 0; i < 9; i++) {
        if (board[i] != 'X' && board[i] != 'O') {
            return false;
        }
    }

    return true;
}

void handle_move(struct mosquitto *mosq, const char *payload) {
    char player;
    int pos;

    if (sscanf(payload, " %c %d", &player, &pos) != 2) {
        publish_status(mosq, "INVALID FORMAT");
        return;
    }

    if (game_over) {
        publish_status(mosq, "GAME OVER. RESET");
        return;
    }

    if (player != 'X' && player != 'O') {
        publish_status(mosq, "INVALID PLAYER");
        return;
    }

    if (player != turn) {
        char msg[32];
        snprintf(msg, sizeof(msg), "NOT %c TURN. TURN %c", player, turn);
        publish_status(mosq, msg);
        return;
    }

    if (pos < 1 || pos > 9) {
        publish_status(mosq, "INVALID MOVE");
        return;
    }

    int index = pos - 1;

    if (board[index] == 'X' || board[index] == 'O') {
        publish_status(mosq, "SPACE TAKEN");
        return;
    }

    board[index] = player;
    publish_state(mosq);

    char winner = check_winner();

    if (winner == 'X' || winner == 'O') {
        char msg[32];
        snprintf(msg, sizeof(msg), "WINNER %c", winner);
        game_over = true;
        publish_status(mosq, msg);
        return;
    }

    if (check_draw()) {
        game_over = true;
        publish_status(mosq, "DRAW");
        return;
    }

    turn = (turn == 'X') ? 'O' : 'X';

    char msg[32];
    snprintf(msg, sizeof(msg), "TURN %c", turn);
    publish_status(mosq, msg);
}

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if (rc == 0) {
        printf("Connected to MQTT broker\n");

        mosquitto_subscribe(mosq, NULL, TOPIC_MOVE, 0);
        mosquitto_subscribe(mosq, NULL, TOPIC_RESET, 0);

        reset_game(mosq);
    } else {
        printf("MQTT connection failed: %d\n", rc);
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    char payload[128];

    memset(payload, 0, sizeof(payload));
    memcpy(payload, msg->payload, msg->payloadlen < 127 ? msg->payloadlen : 127);

    printf("MESSAGE topic=%s payload=%s\n", msg->topic, payload);

    if (strcmp(msg->topic, TOPIC_MOVE) == 0) {
        handle_move(mosq, payload);
    }

    if (strcmp(msg->topic, TOPIC_RESET) == 0) {
        reset_game(mosq);
    }
}

int main() {
    struct mosquitto *mosq;

    printf("Starting C Tic-Tac-Toe MQTT game server...\n");

    mosquitto_lib_init();

    mosq = mosquitto_new("GCP-TTT-Game-Server", true, NULL);

    if (!mosq) {
        fprintf(stderr, "Failed to create Mosquitto client\n");
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Could not connect to MQTT broker\n");
        return 1;
    }

    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}