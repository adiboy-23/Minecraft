#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <stdlib.h>
#include <math.h>

static struct termios old_termios, new_termios;

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
    printf("terminal restored");
}

static char keystate[256]={0};

void process_input(){
    char c;
    for (int i = 0; i < 256; i++) {
        keystate[i] = 0;
    }
    while(read(STDIN_FILENO, &c, 1)>0){
        printf("\ninput char: %c\n", c);
        // if(c=='q'){
        //     printf("quitting...\n");
        //     exit(0);
        // }
        unsigned char uc = (unsigned char)c;
        keystate[uc] = 1;
    }
}

int is_key_pressed(char key) {
    return keystate[(unsigned char)key];
}

int main() { 
    init_terminal();
    while(1){
        process_input();
        if (is_key_pressed('q')) {
            exit(0);
        }
    }
    restore_terminal();
    return 0; 
}
