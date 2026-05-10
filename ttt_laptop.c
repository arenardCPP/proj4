#include <raylib.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MQTT_HOST "arenardcpp.duckdns.org"
#define MQTT_PORT 1883

#define PLAYER "O"

#define TOPIC_MOVE   "ttt/game1/move"
#define TOPIC_STATE  "ttt/game1/state"
#define TOPIC_STATUS "ttt/game1/status"
#define TOPIC_RESET  "ttt/game1/reset"

char board[9] = {'1','2','3','4','5','6','7','8','9'};
char status_msg[64] = "Connecting...";

struct mosquitto *mosq;

// ---------- MQTT ----------

void parse_board(const char *state) {
    char copy[128];
    strncpy(copy, state, sizeof(copy));

    char *token = strtok(copy, "|");
    int i = 0;

    while (token && i < 9) {
        board[i] = token[0];
        token = strtok(NULL, "|");
        i++;
    }
}

void on_connect(struct mosquitto *m, void *obj, int rc) {
    if (rc == 0) {
        printf("Connected to MQTT\n");
        mosquitto_subscribe(m, NULL, TOPIC_STATE, 0);
        mosquitto_subscribe(m, NULL, TOPIC_STATUS, 0);
    }
}

void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg) {
    char payload[128];
    memset(payload, 0, sizeof(payload));
    memcpy(payload, msg->payload, msg->payloadlen < 127 ? msg->payloadlen : 127);

    if (strcmp(msg->topic, TOPIC_STATE) == 0) {
        parse_board(payload);
    }

    if (strcmp(msg->topic, TOPIC_STATUS) == 0) {
        strncpy(status_msg, payload, sizeof(status_msg));
    }
}

// ---------- GAME DRAW ----------

void draw_board() {
    int size = 150;
    int offsetX = 100;
    int offsetY = 100;

    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;

        int x = offsetX + col * size;
        int y = offsetY + row * size;

        DrawRectangleLines(x, y, size, size, BLACK);

        char text[2] = {board[i], '\0'};

        DrawText(text, x + 60, y + 50, 40, BLACK);
    }
}

int get_clicked_cell(Vector2 mouse) {
    int size = 150;
    int offsetX = 100;
    int offsetY = 100;

    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;

        int x = offsetX + col * size;
        int y = offsetY + row * size;

        if (mouse.x > x && mouse.x < x + size &&
            mouse.y > y && mouse.y < y + size) {
            return i + 1;
        }
    }

    return -1;
}

void send_move(int pos) {
    char msg[16];
    snprintf(msg, sizeof(msg), "%s %d", PLAYER, pos);

    mosquitto_publish(mosq, NULL, TOPIC_MOVE, strlen(msg), msg, 0, false);
    printf("Sent: %s\n", msg);
}

// ---------- MAIN ----------

int main() {
    mosquitto_lib_init();

    mosq = mosquitto_new("Raylib-TTT", true, NULL);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        printf("MQTT connection failed\n");
        return 1;
    }

    mosquitto_loop_start(mosq);

    InitWindow(600, 600, "Tic Tac Toe (MQTT)");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            int cell = get_clicked_cell(mouse);

            if (cell != -1) {
                send_move(cell);
            }
        }

        if (IsKeyPressed(KEY_R)) {
            mosquitto_publish(mosq, NULL, TOPIC_RESET, 5, "reset", 0, false);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        draw_board();

        DrawText(status_msg, 50, 20, 20, DARKGRAY);
        DrawText("Click to play | Press R to reset", 50, 550, 18, GRAY);

        EndDrawing();
    }

    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    CloseWindow();

    return 0;
}