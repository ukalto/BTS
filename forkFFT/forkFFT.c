#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <sys/wait.h>
#include <complex.h>
#include <errno.h>
#include <stdbool.h>

#define PI (3.141592654)

bool check_p = false;

char *program_name = "<not set>";

void usage(char * message) {
    fprintf(stderr, "USAGE: %s [-p]\n", program_name);
    exit(EXIT_FAILURE);
}

static void error_exit(char* error_msg) {
    fprintf(stderr, "%s\n", error_msg);
    exit(EXIT_FAILURE);
}

static void error_exit_child(char* error_msg, char *child) {
    fprintf(stderr, "%s %s\n", error_msg, child);
    exit(EXIT_FAILURE);
}

static float complex covert_input_to_complex(char* input, int input_size) {
    char* input_copy = malloc(input_size);
    strcpy(input_copy, input);
    char *realptr, *imaginaryptr;
    
    float real  = strtof(input_copy, &realptr);
    float imaginary = strtof(realptr, &imaginaryptr);
    
    if (errno != 0 || errno == ERANGE) {
        error_exit("Input is wrong, Error"); 
    } else if ((real == 0 && input_copy == imaginaryptr) || (strlen(imaginaryptr) != 0 && *imaginaryptr != '\n' && strcmp(imaginaryptr, "*i\n") != 0)) {
        error_exit("Input is invalid");
    }
    free(input_copy);
        
    float complex result = real + imaginary * I;
    return result;
}

static void print_complex(complex result){
    if(check_p){
        float real_part = creal(result);
        float imag_part = cimag(result);
        char str_real_part[80];
        sprintf(str_real_part, "%.3f", real_part);
        if (strcmp(str_real_part,"-0.000") == 0){
            sprintf(str_real_part, "%s", "0.000");
        }
        char str_imag_part[80];
        sprintf(str_imag_part, "%.3f", imag_part);
        if (strcmp(str_imag_part, "-0.000") == 0){
            sprintf(str_imag_part, "%s", "0.000");
        }
        fprintf(stdout, "%s %s*i\n", str_real_part, str_imag_part);
    } else {
        fprintf(stdout, "%f %f*i\n", creal(result), cimag(result));
    }
}

static void butterfly(char* even_number, char* odd_number, int n, int k, bool negate) {
    float complex c1,c2;
    c1 = covert_input_to_complex(even_number, strlen(even_number));
    c2 = covert_input_to_complex(odd_number, strlen(odd_number));

    float cos_val = cos(-((2*PI)/n)*k);
    float sin_val = sin(-((2*PI)/n)*k);

    float complex result = c1 + (cos_val + sin_val * I) * c2;
    if (negate == true) {
        result =  c1 + (cos_val + sin_val * I) * -1 * c2;
    }
    print_complex(result);
}

static void close_unused_pipes(int fd_in_one[], int fd_in_two[], int fd_out_one[], int fd_out_two[], int in, int out){
    close(fd_in_one[in]);
    close(fd_in_two[in]);
    close(fd_out_one[out]);
    close(fd_out_two[out]);
}

static void check_p_pid(pid_t pid, int fd_in_one[], int fd_out_one[], int fd_in_two[], int fd_out_two[], char *child){
    switch (pid) {
        case -1:
            error_exit("Fork failed");
        case 0:
            if (dup2(fd_out_one[1], STDOUT_FILENO) == -1) error_exit_child("STDOUT dup2 failed, for child", child);
            if (dup2(fd_in_one[0], STDIN_FILENO) == -1) error_exit_child("STDIN dup2 failed, for child", child);

            close_unused_pipes(fd_in_one, fd_in_two, fd_out_one, fd_out_two, 0, 1);
            close_unused_pipes(fd_in_one, fd_in_two, fd_out_one, fd_out_two, 1, 0);

            if (execlp(program_name, program_name, NULL) == -1){
                error_exit_child("Failed execlp for child", child);
            } 
            break;
        default:
            break;
    }
}

static void write_to_child(FILE **write_one, FILE **write_two, int fd_in_one, int fd_in_two, char *part_even, char *part_odd){
    if ((*write_one = fdopen(fd_in_one, "w")) == NULL) {
        error_exit("Error opening write 1 can't write to pipe 1");
    } else if ((*write_two = fdopen(fd_in_two, "w")) == NULL) {
        error_exit("Error opening write 2 can't write to pipe 2");
    } else if (fprintf(*write_one, "%s", part_even) < 0) {
        error_exit("Writing to child 1 failed");
    } else if (fprintf(*write_two, "%s", part_odd) < 0) {
        error_exit("Writing to child 2 failed");
    }
}

static void free_r2(int k, char **r2_even, char **r2_odd){
    for(int j = 0; j < k; j++) {
        free(r2_even[j]);
        free(r2_odd[j]);
    }
}

int main(int argc, char *argv[]) {
    if(argc == 2) check_p = true;

    program_name = argv[0];

    if(argc > 2) usage("Invalid arguments");

    char *part_even, *part_odd, *valueptr;

    int fd_in_one[2], fd_out_one[2], fd_in_two[2], fd_out_two[2];

    size_t line_size, line_size_child = 0;
    
    int line_val, n, k = 0;

    while(n < 2 && (line_val = getline(&valueptr, &line_size, stdin)) != EOF) {
        if (line_val == -1) {
            error_exit("Failed to read from stdin");
        } 
        switch(n % 2){
            case 0:
                part_even = realloc(part_even, (strlen(valueptr)*sizeof(char*)));
                strcpy(part_even, valueptr);
                break;
            case 1:
                part_odd = realloc(part_odd, (strlen(valueptr)*sizeof(char*)));
                strcpy(part_odd, valueptr);
                break;
        } 
        n++;
    }
    if (n < 1) {
        error_exit("Can't process any value");
    } else if (n == 1) {
        float complex result = covert_input_to_complex(part_even, strlen(part_even));
        print_complex(result);
        exit(EXIT_SUCCESS);
    } else if (n % 2 != 0) {
        error_exit("2^n arguments excpected");
    }

    int child_one_pipe_in, child_one_pipe_out, child_two_pipe_in, child_two_pipe_out;

    if ((child_one_pipe_in = pipe(fd_in_one)) != 0) error_exit("pipe in from child 1 failed");
    if ((child_one_pipe_out = pipe(fd_out_one)) != 0) error_exit("pipe out from child 1 failed");
    if ((child_two_pipe_in = pipe(fd_in_two)) != 0) error_exit("pipe in from child 2 failed");
    if ((child_two_pipe_out = pipe(fd_out_two)) != 0) error_exit("pipe out from child 2 failed");


    pid_t pid_one = fork(); 
    check_p_pid(pid_one, fd_in_one, fd_out_one, fd_in_two, fd_out_two, "1");
    pid_t pid_two = fork();
    check_p_pid(pid_two, fd_in_two, fd_out_two, fd_in_one, fd_out_one, "2");

    close_unused_pipes(fd_in_one, fd_in_two, fd_out_one, fd_out_two, 0, 1);

    FILE *write_one, *write_two = NULL;
    write_to_child(&write_one, &write_two, fd_in_one[1], fd_in_two[1], part_even, part_odd);
    
    while (getline(&part_even, &line_size_child, stdin) != -1)  {
        if (fprintf(write_one, "%s", part_even) < 0) error_exit("Failed to write from part_even to child 1");
        n++;
        if (getline(&part_odd, &line_size_child, stdin) == -1) break;
        if (fprintf(write_two, "%s", part_odd) < 0) error_exit("Failed to write from part_odd to child 2");
        n++;
    }

    fclose(write_one);
    fclose(write_two);

    int status;
    while (waitpid(pid_one, &status, 0) == -1) {error_exit("Can't wait for child 1");}
    if (WEXITSTATUS(status) != EXIT_SUCCESS) error_exit("Wait pid 1 failed, Child 1: EXIT_FAILURE");
    while (waitpid(pid_two, &status, 0) == -1) {error_exit("Can't wait for child 2");}
    if (WEXITSTATUS(status) != EXIT_SUCCESS) error_exit("Wait pid 2 failed, Child 2: EXIT_FAILURE");    

    FILE *read_one, *read_two;
    if ((read_one = fdopen(fd_out_one[0], "r")) == NULL) error_exit("Error: Can't read from child 1");
    else if ((read_two = fdopen(fd_out_two[0], "r")) == NULL) error_exit("Error: Can't read from child 2");

    char *r2_even[n/2], *r2_odd[n/2];
    int r2_size = n/2;
    while (strcmp(part_even, "\n") != 0 && (line_val = getline(&part_even, &line_size_child, read_one)) != EOF) { 
        if (line_val == -1 || getline(&part_odd, &line_size_child, read_two) == -1) {
            free_r2(k, r2_even, r2_odd);
            error_exit("Reading from children failed");
        }
        
        butterfly(part_even, part_odd, n, k, false);
        
        r2_even[k] = malloc(sizeof(char*)*r2_size);
        r2_odd[k] = malloc(sizeof(char*)*r2_size);
        strcpy(r2_even[k], part_even);
        strcpy(r2_odd[k], part_odd);
        k++;
    } 
    
    k = 0;
    for(int j = 0; j < n/2; j++) {
        butterfly(r2_even[j], r2_odd[j], n, k, true);
        k++;
    }

    free_r2(k, r2_even, r2_odd);

    close_unused_pipes(fd_in_one, fd_in_two, fd_out_one, fd_out_two, 1, 0);

    free(part_even);
    free(part_odd);
    free(valueptr);

    fclose(read_one);
    fclose(read_two);

    exit(EXIT_SUCCESS);
}
