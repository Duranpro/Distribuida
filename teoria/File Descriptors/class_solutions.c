//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************
#define _GNU_SOURCE
#include <stdlib.h> // For atoi
#include <string.h> // For strlen
#include <unistd.h> // For read and write
#include <fcntl.h>  // For open, close
#include <stdio.h>

void exercise1(){
    int num;
    write(1, "Number: ", strlen("Number: "));
    read(0, &num, sizeof(int));
    
    char *output;
    int len = asprintf(&output, "You entered the number: %d\n", num);
    write(1, output, len);
    free(output);
}

void exercise2(){
    char c;
    write(1, "Character: ", strlen("Character: "));
    read(0, &c, 1);
    char *output;
    int len = asprintf(&output, "You entered the character: %c\n", c);
    write(1, output, len);
    free(output);
}

void exercise3(){
    char buffer[20];
    write(1, "Number: ", strlen("Number: "));
    int bytes = read(0, buffer, sizeof(buffer));
    buffer[bytes - 1] = '\0'; // Remove the \n
    float num = atof(buffer);
    char *output;
    int len = asprintf(&output, "You entered the decimal number: %.2f\n", num);
    write(1, output, len);
    free(output);
}

void exercise4(){
    char buffer[10];
    write(1, "First number: ", strlen("First number: "));
    int bytes = read(0, buffer, sizeof(buffer));
    buffer[bytes - 1] = '\0'; // Remove the \n
    int num1 = atoi(buffer);

    write(1, "Second number: ", strlen("Second number: "));
    bytes = read(0, buffer, sizeof(buffer));
    buffer[bytes - 1] = '\0';
    int num2 = atoi(buffer);

    int suma = num1 + num2;
    char *output;
    int len = asprintf(&output, "The sum is: %d\n", suma);
    write(1, output, len);
    free(output);
}

void exercise5(){
    char text[100], car;
    write(1, "String: ", strlen("String: "));
    int bytes = read(0, text, sizeof(text));
    text[bytes - 1] = '\0'; // Remove the \n

    write(1, "Character: ", strlen("Character: "));
    read(0, &car, 1);

    int count = 0;
    for (int i = 0; i < (int)strlen(text); i++) {
        if (text[i] == car) {
            count++;
        }
    }

    char *output;
    int len = asprintf(&output, "The character \"%c\" appears %d time(s).\n", car, count);
    write(1, output, len);
    free(output);
}

void exercise6(){
    char buffer[10];
    write(1, "Number: ", strlen("Number: "));
    int bytes = read(0, buffer, sizeof(buffer));
    buffer[bytes - 1] = '\0'; // Remove the \n
    int num = atoi(buffer);

    int fd = open("number.txt", O_WRONLY | O_CREAT, 0644);
    char output[100];
    int len = sprintf(output, "%d\n", num);
    write(fd, output, len);
    close(fd);

    char *screen;
    len = asprintf(&screen, "The number %d has been written to the file \"number.txt\".\n", num);
    write(1, screen, len);
    free(screen);
}

void exercise7(){
    char text[100];
    write(1, "String: ", strlen("String: "));
    int bytes = read(0, text, sizeof(text));
    text[bytes - 1] = '\0'; // Remove the \n

    int fd = open("text.txt", O_WRONLY | O_CREAT, 0644);
    write(fd, text, strlen(text));
    close(fd);

    char *output;
    int len = asprintf(&output, "\"%s\" has been written to the file \"text.txt\".\n", text);
    write(1, output, len);
    free(output);
}

void exercise8(){
    char buffer[10];
    int fd = open("number.txt", O_RDONLY);
    int bytes = read(fd, buffer, sizeof(buffer));
    buffer[bytes] = '\0';
    close(fd);

    char *output;
    int len = asprintf(&output, "The number is: %s\n", buffer);
    write(1, output, len);
    free(output);
}

void exercise9(){
    char buffer[100];
    write(1, "String: ", strlen("String: "));
    int bytes = read(0, buffer, sizeof(buffer));
    buffer[bytes - 1] = '\0'; // Remove the \n

    char *output;
    int len = asprintf(&output, "The string was: %s\n", buffer);
    write(1, output, len);
    free(output);
}

int main(int argc, char *argv[]){

    if(argc != 2){
        write(1, "Missing exercise number\n", strlen("Missing exercise number\n"));
        return 1;
    }

    switch(atoi(argv[1])){
        case 1:
            exercise1();
            break;
        case 2:
            exercise2();
            break;
        case 3:
            exercise3();
            break;
        case 4:
            exercise4();
            break;
        case 5:
            exercise5();
            break;
        case 6:
            exercise6();
            break;
        case 7:
            exercise7();
            break;
        case 8:
            exercise8();
            break;
        case 9:
            exercise9();
            break;
        default:
            write(1, "Invalid option\n", strlen("Invalid option\n"));
            break;
    }

    return 0;

}
