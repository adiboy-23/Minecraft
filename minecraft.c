#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <stdlib.h>
#include <math.h>

#define Y_PIXELS 40        // Reduced for terminal readability
#define X_PIXELS 100       // Reduced for terminal readability
#define Z_BLOCKS 10
#define Y_BLOCKS 20
#define X_BLOCKS 20
#define EYE_HEIGHT 1.5
#define VIEW_HEIGHT 0.7
#define VIEW_WIDTH 1
#define BLOCK_BORDER_SIZE 0.05

static struct termios old_termios, new_termios;

typedef struct Vector {
    float x;
    float y;
    float z;
} vect;

typedef struct Vector2 {
    float psi;
    float phi;
} vect2;

typedef struct Vector_vector2 {
    vect pos;
    vect2 view;
} player_pos_view;

// Vector math
vect angles_to_vect(vect2 angles) {
    vect res;
    res.x = cosf(angles.psi) * cosf(angles.phi);
    res.y = cosf(angles.psi) * sinf(angles.phi);
    res.z = sinf(angles.psi);
    return res;
}

vect vect_add(vect v1, vect v2) {
    vect res = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
    return res;
}

vect vect_scale(float s, vect v) {
    vect res = { s * v.x, s * v.y, s * v.z };
    return res;
}

vect vect_sub(vect v1, vect v2) {
    return vect_add(v1, vect_scale(-1, v2));
}

void vect_normalize(vect* v) {
    float len = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    if (len != 0) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
}

// Terminal I/O
void init_terminal() {
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
    fflush(stdout);
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    printf("terminal restored\n");
}

// Keyboard state
static char keystate[256] = {0};

void process_input() {
    char c;
    for (int i = 0; i < 256; i++) keystate[i] = 0;
    while (read(STDIN_FILENO, &c, 1) > 0) {
        keystate[(unsigned char)c] = 1;
    }
}

int is_key_pressed(char key) {
    return keystate[(unsigned char)key];
}

// Scene setup
char** init_picture() {
    char** picture = malloc(sizeof(char*) * Y_PIXELS);
    for (int i = 0; i < Y_PIXELS; i++) {
        picture[i] = malloc(sizeof(char) * X_PIXELS);
    }
    return picture;
}

char*** init_blocks() {
    char*** blocks = malloc(sizeof(char**) * Z_BLOCKS);
    for (int i = 0; i < Z_BLOCKS; i++) {
        blocks[i] = malloc(sizeof(char*) * Y_BLOCKS);
        for (int j = 0; j < Y_BLOCKS; j++) {
            blocks[i][j] = malloc(sizeof(char) * X_BLOCKS);
            for (int k = 0; k < X_BLOCKS; k++) {
                blocks[i][j][k] = ' ';
            }
        }
    }
    return blocks;
}

// Ray functions
int ray_outside(vect pos) {
    return (pos.x < 0 || pos.y < 0 || pos.z < 0 ||
            pos.x >= X_BLOCKS || pos.y >= Y_BLOCKS || pos.z >= Z_BLOCKS);
}

int on_block_border(vect pos) {
    int cnt = 0;
    if (fabsf(pos.x - roundf(pos.x)) < BLOCK_BORDER_SIZE) cnt++;
    if (fabsf(pos.y - roundf(pos.y)) < BLOCK_BORDER_SIZE) cnt++;
    if (fabsf(pos.z - roundf(pos.z)) < BLOCK_BORDER_SIZE) cnt++;
    return cnt >= 2;
}

float minf(float a, float b) {
    return (a < b) ? a : b;
}

char raytrace(vect pos, vect dir, char*** blocks) {
    const float eps = 0.01;
    while (!ray_outside(pos)) {
        char c = blocks[(int)pos.z][(int)pos.y][(int)pos.x];
        if (c != ' ') {
            return on_block_border(pos) ? '-' : c;
        }

        float dist = 2;
        if (dir.x > eps) dist = minf(dist, ((int)(pos.x + 1) - pos.x) / dir.x);
        else if (dir.x < -eps) dist = minf(dist, ((int)pos.x - pos.x) / dir.x);
        if (dir.y > eps) dist = minf(dist, ((int)(pos.y + 1) - pos.y) / dir.y);
        else if (dir.y < -eps) dist = minf(dist, ((int)pos.y - pos.y) / dir.y);
        if (dir.z > eps) dist = minf(dist, ((int)(pos.z + 1) - pos.z) / dir.z);
        else if (dir.z < -eps) dist = minf(dist, ((int)pos.z - pos.z) / dir.z);

        pos = vect_add(pos, vect_scale(dist + eps, dir));
    }
    return ' ';
}

// Direction vectors per pixel
vect** init_directions(vect2 view) {
    vect2 v = view;
    v.psi -= VIEW_HEIGHT / 2.0;
    vect down = angles_to_vect(v);
    v.psi += VIEW_HEIGHT;
    vect up = angles_to_vect(v);
    v.psi -= VIEW_HEIGHT / 2.0;

    v.phi -= VIEW_WIDTH / 2.0;
    vect left = angles_to_vect(v);
    v.phi += VIEW_WIDTH;
    vect right = angles_to_vect(v);

    vect mid_vert = vect_scale(0.5, vect_add(up, down));
    vect mid_hor = vect_scale(0.5, vect_add(left, right));
    vect delta_left = vect_sub(left, mid_hor);
    vect delta_up = vect_sub(up, mid_vert);

    vect** dir = malloc(sizeof(vect*) * Y_PIXELS);
    for (int i = 0; i < Y_PIXELS; i++) {
        dir[i] = malloc(sizeof(vect) * X_PIXELS);
        for (int j = 0; j < X_PIXELS; j++) {
            vect tmp = vect_add(mid_hor, delta_left);
            tmp = vect_add(tmp, delta_up);
            tmp = vect_sub(tmp, vect_scale(((float)j / (X_PIXELS - 1)) * 2, delta_left));
            tmp = vect_sub(tmp, vect_scale(((float)i / (Y_PIXELS - 1)) * 2, delta_up));
            vect_normalize(&tmp);
            dir[i][j] = tmp;
        }
    }
    return dir;
}

player_pos_view init_posview() {
    player_pos_view pv;
    pv.pos.x = 5;
    pv.pos.y = 5;
    pv.pos.z = 5;
    pv.view.psi = 0;
    pv.view.phi = 0;
    return pv;
}

void get_picture(char** picture, player_pos_view posview, char*** blocks) {
    vect** directions = init_directions(posview.view);
    for (int y = 0; y < Y_PIXELS; y++) {
        for (int x = 0; x < X_PIXELS; x++) {
            picture[y][x] = raytrace(posview.pos, directions[y][x], blocks);
        }
    }

    // Free allocated direction vectors
    for (int i = 0; i < Y_PIXELS; i++) {
        free(directions[i]);
    }
    free(directions);
}

void draw_ascii(char** picture) {
    printf("\033[0;0H");  // Reset cursor position
    for (int i = 0; i < Y_PIXELS; i++) {
        int current_color = 0;
        for (int j = 0; j < X_PIXELS; j++) {
            if (picture[i][j] == 'o' && current_color != 32) {
                printf("\x1B[32m");
                current_color = 32;
            } else if (picture[i][j] != 'o' && current_color != 0) {
                printf("\x1B[0m");
                current_color = 0;
            }
            printf("%c", picture[i][j]);
        }
        printf("\x1B[0m\n");
    }
}

int main() {
    init_terminal();
    atexit(restore_terminal);

    char** picture = init_picture();
    char*** blocks = init_blocks();

    // Fill lower blocks
    for (int x = 0; x < X_BLOCKS; x++) {
        for (int y = 0; y < Y_BLOCKS; y++) {
            for (int z = 0; z < 4; z++) {
                blocks[z][y][x] = '@';
            }
        }
    }

    player_pos_view posview = init_posview();

    while (1) {
        process_input();
        if (is_key_pressed('q')) break;

        get_picture(picture, posview, blocks);
        draw_ascii(picture);
        usleep(33000);  // ~30 FPS
    }

    return 0;
}