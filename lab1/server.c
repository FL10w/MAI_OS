#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

float bober(char buf[]) {
    float numbers[100];
    int count = 0;
    int i = 0;
    
    while (buf[i] != '\0' && buf[i] != '\n') {
        while (buf[i] == ' ') i++;
        
        if (buf[i] == '\0' || buf[i] == '\n') break;

        float num = 0;
        int is_negative = 0;
        int has_decimal = 0;
        float decimal_place = 0.1f;
        
        if (buf[i] == '-') {
            is_negative = 1;
            i++;
        }
        
        while ((buf[i] >= '0' && buf[i] <= '9') || buf[i] == '.') {
            if (buf[i] == '.') {
                has_decimal = 1;
            } else {
                if (!has_decimal) {
                    num = num * 10 + (buf[i] - '0');
                } else {
                    num += (buf[i] - '0') * decimal_place;
                    decimal_place *= 0.1f;
                }
            }
            i++;
        }
        
        if (is_negative) {
            num = -num;
        }
        
        numbers[count++] = num;
        while (buf[i] == ' ') i++;
    }
    
    if (count == 0) {
        return 0;
    }
    
    if (count == 1) {
        return numbers[0];
    }
    
    float result = numbers[0];
    for (int j = 1; j < count; j++) {
        if (numbers[j] == 0) {
            const char msg[] = "error: division by zero\n";
            write(STDOUT_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        result /= numbers[j];
    }
    
    return result;
}
int main(int argc, char **argv) {
	char buf[4096];
	ssize_t bytes;
	int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (file == -1) {
		const char msg[] = "error: failed to open requested file\n";
	    write(STDOUT_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	while ((bytes = read(STDIN_FILENO, buf, sizeof(buf)))) {
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDOUT_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

        float result = bober(buf);
        bytes = snprintf(buf, sizeof(buf), "%.4f\n", result);
		{
			int32_t written = write(file, buf, bytes);
			if (written != bytes) {
				const char msg[] = "error: failed to write to file\n";
				write(STDOUT_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}       
		}
        const char msg[] = "success\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
	}
	if (bytes == 0) {
		const char term = '\0';
		write(file, &term, sizeof(term));
	}

	close(file);
}