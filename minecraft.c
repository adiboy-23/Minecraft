#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <stdlib.h>
#include <math.h>

// Display and world configuration
#define DISPLAY_HEIGHT 180
#define DISPLAY_WIDTH 900
#define WORLD_DEPTH 10
#define WORLD_HEIGHT 20
#define PLAYER_EYE_LEVEL 1.5
#define WORLD_WIDTH 20
#define FIELD_OF_VIEW_VERTICAL 0.7
#define FIELD_OF_VIEW_HORIZONTAL 1.0
#define EDGE_THRESHOLD 0.05

// Terminal control structures
static struct termios original_terminal_settings, modified_terminal_settings;

// 3D vector representation
typedef struct Point3D {
    float x;
    float y;
    float z;
} Point3D;

// View angles representation
typedef struct ViewAngles {
    float pitch;
    float yaw;
} ViewAngles;

// Player state structure
typedef struct PlayerState {
    Point3D position;
    ViewAngles orientation;
} PlayerState;

// Input handling
static char input_buffer[256] = { 0 };

// Terminal setup functions
void configure_terminal() {
    tcgetattr(STDIN_FILENO, &original_terminal_settings);
    modified_terminal_settings = original_terminal_settings;
    modified_terminal_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &modified_terminal_settings);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
    fflush(stdout);
}

void reset_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal_settings);
    printf("Terminal settings restored\n");
}

// Input processing
void handle_input() {
    char input_char;
    for (int i = 0; i < 256; i++) {
        input_buffer[i] = 0;
    }

    while (read(STDIN_FILENO, &input_char, 1) > 0) {
        unsigned char key_code = (unsigned char)input_char;
        input_buffer[key_code] = 1;
    }
}

int check_key_press(char key) {
    return input_buffer[(unsigned char)key];
}

// Memory allocation for display and world
char** create_display_buffer() {
    char** display = malloc(sizeof(char*) * DISPLAY_HEIGHT);
    for (int i = 0; i < DISPLAY_HEIGHT; i++) {
        display[i] = malloc(sizeof(char) * DISPLAY_WIDTH);
    }
    return display;
}

char*** create_world() {
    char*** world = malloc(sizeof(char**) * WORLD_DEPTH);
    for (int i = 0; i < WORLD_DEPTH; i++) {
        world[i] = malloc(sizeof(char*) * WORLD_HEIGHT);
        for (int j = 0; j < WORLD_HEIGHT; j++) {
            world[i][j] = malloc(sizeof(char) * WORLD_WIDTH);
            for (int k = 0; k < WORLD_WIDTH; k++) {
                world[i][j][k] = ' ';
            }
        }
    }
    return world;
}

// Player initialization
PlayerState initialize_player() {
    PlayerState player;
    player.position.x = 5;
    player.position.y = 5;
    player.position.z = 4 + PLAYER_EYE_LEVEL;
    player.orientation.yaw = 0;
    player.orientation.pitch = 0;
    return player;
}

// Vector operations
Point3D convert_angles_to_vector(ViewAngles angles) {
    Point3D result;
    result.x = cos(angles.pitch) * cos(angles.yaw);
    result.y = cos(angles.pitch) * sin(angles.yaw);
    result.z = sin(angles.pitch);
    return result;
}

Point3D add_vectors(Point3D v1, Point3D v2) {
    Point3D result;
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    return result;
}

Point3D scale_vector(float scalar, Point3D v) {
    Point3D result = { scalar * v.x, scalar * v.y, scalar * v.z };
    return result;
}

Point3D subtract_vectors(Point3D v1, Point3D v2) {
    Point3D neg_v2 = scale_vector(-1, v2);
    return add_vectors(v1, neg_v2);
}

void normalize_vector(Point3D* v) {
    float magnitude = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x /= magnitude;
    v->y /= magnitude;
    v->z /= magnitude;
}

// View direction calculation
Point3D** calculate_view_directions(ViewAngles view) {
    view.pitch -= FIELD_OF_VIEW_VERTICAL / 2.0;
    Point3D bottom_edge = convert_angles_to_vector(view);
    view.pitch += FIELD_OF_VIEW_VERTICAL;
    Point3D top_edge = convert_angles_to_vector(view);
    view.pitch -= FIELD_OF_VIEW_VERTICAL / 2.0;
    view.yaw -= FIELD_OF_VIEW_HORIZONTAL / 2.0;
    Point3D left_edge = convert_angles_to_vector(view);
    view.yaw += FIELD_OF_VIEW_HORIZONTAL;
    Point3D right_edge = convert_angles_to_vector(view);
    view.yaw -= FIELD_OF_VIEW_HORIZONTAL / 2.0;

    Point3D vertical_center = scale_vector(0.5, add_vectors(top_edge, bottom_edge));
    Point3D horizontal_center = scale_vector(0.5, add_vectors(left_edge, right_edge));
    Point3D horizontal_offset = subtract_vectors(left_edge, horizontal_center);
    Point3D vertical_offset = subtract_vectors(top_edge, vertical_center);

    Point3D** directions = malloc(sizeof(Point3D*) * DISPLAY_HEIGHT);
    for (int i = 0; i < DISPLAY_HEIGHT; i++) {
        directions[i] = malloc(sizeof(Point3D) * DISPLAY_WIDTH);
    }

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            Point3D direction = add_vectors(add_vectors(horizontal_center, horizontal_offset), vertical_offset);
            direction = subtract_vectors(direction, scale_vector(((float)x / (DISPLAY_WIDTH - 1)) * 2, horizontal_offset));
            direction = subtract_vectors(direction, scale_vector(((float)y / (DISPLAY_HEIGHT - 1)) * 2, vertical_offset));
            normalize_vector(&direction);
            directions[y][x] = direction;
        }
    }
    return directions;
}

// World boundary checks
int is_outside_world(Point3D pos) {
    return (pos.x >= WORLD_WIDTH || pos.y >= WORLD_HEIGHT || pos.z >= WORLD_DEPTH
            || pos.x < 0 || pos.y < 0 || pos.z < 0);
}

int is_on_block_edge(Point3D pos) {
    int edge_count = 0;
    if (fabsf(pos.x - roundf(pos.x)) < EDGE_THRESHOLD) edge_count++;
    if (fabsf(pos.y - roundf(pos.y)) < EDGE_THRESHOLD) edge_count++;
    if (fabsf(pos.z - roundf(pos.z)) < EDGE_THRESHOLD) edge_count++;
    return edge_count >= 2;
}

float get_minimum(float a, float b) {
    return (a < b) ? a : b;
}

// Ray tracing implementation
char cast_ray(Point3D pos, Point3D dir, char*** world) {
    float epsilon = 0.01;
    while (!is_outside_world(pos)) {
        char block = world[(int)pos.z][(int)pos.y][(int)pos.x];
        if (block != ' ') {
            return is_on_block_edge(pos) ? '-' : block;
        }
        
        float distance = 2;
        if (dir.x > epsilon) {
            distance = get_minimum(distance, ((int)(pos.x + 1) - pos.x) / dir.x);
        }
        else if (dir.x < -epsilon) {
            distance = get_minimum(distance, ((int)pos.x - pos.x) / dir.x);
        }
        if (dir.y > epsilon) {
            distance = get_minimum(distance, ((int)(pos.y + 1) - pos.y) / dir.y);
        }
        else if (dir.y < -epsilon) {
            distance = get_minimum(distance, ((int)pos.y - pos.y) / dir.y);
        }
        if (dir.z > epsilon) {
            distance = get_minimum(distance, ((int)(pos.z + 1) - pos.z) / dir.z);
        }
        else if (dir.z < -epsilon) {
            distance = get_minimum(distance, ((int)pos.z - pos.z) / dir.z);
        }
        pos = add_vectors(pos, scale_vector(distance + epsilon, dir));
    }
    return ' ';
}

// Rendering functions
void render_frame(char** display, PlayerState player, char*** world) {
    Point3D** view_directions = calculate_view_directions(player.orientation);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            display[y][x] = cast_ray(player.position, view_directions[y][x], world);
        }
    }
}

void render_display(char** display) {
    fflush(stdout);
    printf("\033[0;0H");
    for (int i = 0; i < DISPLAY_HEIGHT; i++) {
        int active_color = 0;
        for (int j = 0; j < DISPLAY_WIDTH; j++) {
            if (display[i][j] == 'o' && active_color != 32) {
                printf("\x1B[32m");
                active_color = 32;
            }
            else if (display[i][j] != 'o' && active_color != 0) {
                printf("\x1B[0m");
                active_color = 0;
            }
            printf("%c", display[i][j]);
        }
        printf("\x1B[0m\n");
    }
}

// Player movement and interaction
void update_player_state(PlayerState* player, char*** world) {
    float movement_speed = 0.30;
    float rotation_speed = 0.1;
    int x = (int)player->position.x, y = (int)player->position.y;
    int z = (int)player->position.z - PLAYER_EYE_LEVEL + 0.01;
    
    if (world[z][y][x] != ' ') {
        player->position.z += 1;
    }
    z = (int)player->position.z - PLAYER_EYE_LEVEL - 0.01;
    if (world[z][y][x] == ' ') {
        player->position.z -= 1;
    }

    if (check_key_press('w')) player->orientation.pitch += rotation_speed;
    if (check_key_press('s')) player->orientation.pitch -= rotation_speed;
    if (check_key_press('d')) player->orientation.yaw += rotation_speed;
    if (check_key_press('a')) player->orientation.yaw -= rotation_speed;

    Point3D movement_direction = convert_angles_to_vector(player->orientation);
    
    if (check_key_press('j')) {
        player->position.x += movement_speed * movement_direction.x;
        player->position.y += movement_speed * movement_direction.y;
    }
    if (check_key_press('k')) {
        player->position.x -= movement_speed * movement_direction.x;
        player->position.y -= movement_speed * movement_direction.y;
    }
    if (check_key_press('h')) {
        player->position.x += movement_speed * movement_direction.y;
        player->position.y -= movement_speed * movement_direction.x;
    }
    if (check_key_press('l')) {
        player->position.x -= movement_speed * movement_direction.y;
        player->position.y += movement_speed * movement_direction.x;
    }
}

// Block interaction
Point3D get_targeted_block(PlayerState player, char*** world) {
    Point3D pos = player.position;
    Point3D dir = convert_angles_to_vector(player.orientation);
    float epsilon = 0.01;
    
    while (!is_outside_world(pos)) {
        char block = world[(int)pos.z][(int)pos.y][(int)pos.x];
        if (block != ' ') {
            return pos;
        }
        
        float distance = 2;
        if (dir.x > epsilon) {
            distance = get_minimum(distance, ((int)(pos.x + 1) - pos.x) / dir.x);
        }
        else if (dir.x < -epsilon) {
            distance = get_minimum(distance, ((int)pos.x - pos.x) / dir.x);
        }
        if (dir.y > epsilon) {
            distance = get_minimum(distance, ((int)(pos.y + 1) - pos.y) / dir.y);
        }
        else if (dir.y < -epsilon) {
            distance = get_minimum(distance, ((int)pos.y - pos.y) / dir.y);
        }
        if (dir.z > epsilon) {
            distance = get_minimum(distance, ((int)(pos.z + 1) - pos.z) / dir.z);
        }
        else if (dir.z < -epsilon) {
            distance = get_minimum(distance, ((int)pos.z - pos.z) / dir.z);
        }
        pos = add_vectors(pos, scale_vector(distance + epsilon, dir));
    }
    return pos;
}

void place_block_at(Point3D pos, char*** world, char block_type) {
    int x = (int)pos.x, y = (int)pos.y, z = (int)pos.z;
    float distances[6];
    distances[0] = fabsf(x + 1 - pos.x);
    distances[1] = fabsf(pos.x - x);
    distances[2] = fabsf(y + 1 - pos.y);
    distances[3] = fabsf(pos.y - y);
    distances[4] = fabsf(z + 1 - pos.z);
    distances[5] = fabsf(pos.z - z);
    
    int nearest_face = 0;
    float min_distance = distances[0];
    for (int i = 0; i < 6; i++) {
        if (distances[i] < min_distance) {
            min_distance = distances[i];
            nearest_face = i;
        }
    }
    
    switch (nearest_face) {
        case 0: world[z][y][x + 1] = block_type; break;
        case 1: world[z][y][x - 1] = block_type; break;
        case 2: world[z][y + 1][x] = block_type; break;
        case 3: world[z][y - 1][x] = block_type; break;
        case 4: world[z + 1][y][x] = block_type; break;
        case 5: world[z - 1][y][x] = block_type; break;
    }
}

int main() {
    configure_terminal();
    char** display_buffer = create_display_buffer();
    char*** game_world = create_world();
    
    // Initialize world with ground blocks
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            for (int z = 0; z < 4; z++) {
                game_world[z][y][x] = '@';
            }
        }
    }
    
    PlayerState player = initialize_player();
    
    while (1) {
        handle_input();
        if (check_key_press('q')) {
            break;
        }
        
        update_player_state(&player, game_world);
        Point3D target_block = get_targeted_block(player, game_world);
        int valid_target = !is_outside_world(target_block);
        int target_x = target_block.x;
        int target_y = target_block.y;
        int target_z = target_block.z;
        char target_block_type;
        int block_removed = 0;
        
        if (valid_target) {
            target_block_type = game_world[target_z][target_y][target_x];
            game_world[target_z][target_y][target_x] = 'o';
            
            if (check_key_press('x')) {
                block_removed = 1;
                game_world[target_z][target_y][target_x] = ' ';
            }
            
            if (check_key_press(' ')) {
                place_block_at(target_block, game_world, '@');
            }
        }
        
        render_frame(display_buffer, player, game_world);
        
        if (valid_target && !block_removed) {
            game_world[target_z][target_y][target_x] = target_block_type;
        }
        
        render_display(display_buffer);
        usleep(20000);
    }
    
    reset_terminal();
    return 0;
}