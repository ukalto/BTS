/**
 * @file client.c
 * @author Maximilian Gaber 52009273
 * @brief HTTP Client which requests a file that is specified in the url and writes the response into stdout or a file
 * @version 0.1
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>

static char *program_name = "<not set>";
FILE *out_file, *socket_file;

typedef struct client_url
{
	char *hostname;
	char *file;
	char *dir;
} client_url;

typedef struct client_input
{
	char *port;
	char *file;
	char *dir;
	client_url url;
} client_input;

/**
 * @brief Prints how the programm should be executed and exits with failure called when wrong input
 *
 */
static void usage(void)
{
	fprintf(stderr, "[%s] Usage: [-p PORT] [-o FILE | -d DIR] URL\n", program_name);
	exit(EXIT_FAILURE);
}

/**
 * @brief Prints an error with an error message and calles usage
 *
 * @param message is the error message depending when its called
 */
static void wrong_input(char *message)
{
	fprintf(stderr, "[%s] ERROR: %s\n", program_name, message);
	usage();
}

/**
 * @brief Prints an error with a specific error message
 *
 * @param message is the error message depending when its called
 */
static void exit_with_error(char *message)
{
	fprintf(stderr, "[%s] ERROR: %s\n", program_name, message);
	exit(EXIT_FAILURE);
}

/**
 * @brief checks if the given filename and given dirname are both set which would be wrong
 * and the an error would be printed and usage would be called
 *
 * @param filename can be null and represents the name of the file
 * @param dirname can be null and represents the name of the directory
 */
static void check_execution(char *filename, char *dirname)
{
	if (dirname != NULL && filename != NULL)
	{
		fprintf(stderr, "[%s] ERROR: -d and -o cannot be set together\n", program_name);
		usage();
	}
}

/**
 * @brief is called when exiting the program to cleanup both FILE pointers
 *
 */
static void clean_up()
{
	if (out_file != NULL)
	{
		fclose(out_file);
	}
	if (socket_file != NULL)
	{
		fclose(socket_file);
	}
}

/**
 * @brief parses the url and splits it into 3 parts (host, directory, file)
 * First it is checked if the url starts adequate "http://"
 * Then the url is splited and every variable is safed in our struct which is then at the end returned
 *
 * @param url is the given url
 * @return client_url
 */
static client_url parse_url(char *url)
{
	if (strncmp(url, "http://", 7) != 0)
	{
		fprintf(stderr, "[%s] ERROR: Invalid URL %s must start with 'http://'", program_name, url);
		usage();
	}
	client_url client_url;
	char *file = "index.html";
	// gets the directory after http://
	char *directory = &url[7];
	// gets the hostnamename by seperating the directory when one of the ;/?:@=& symbols occure
	client_url.hostname = strsep(&directory, ";/?:@=&");

	if (directory != NULL)
	{
		file = strrchr(directory, '/');
		if (file == NULL)
		{
			file = directory;
		}
		if (strcmp(file, "") == 0 || strcmp(file, "/") == 0)
		{
			file = "index.html";
		}
		if (file[0] == '/')
		{
			file = &file[1];
		}
		client_url.dir = directory;
		client_url.file = file;
	}
	else
	{
		client_url.dir = "";
		client_url.file = file;
	}
	return client_url;
}

/**
 * @brief opens the specified file where the requested file content should end up
 *
 * @param client_input
 */
static void open_file(client_input client_input)
{
	out_file = stdout;
	if (client_input.file != NULL)
	{
		out_file = fopen(client_input.file, "w");
	}
	else if (client_input.dir != NULL)
	{
		size_t dirlen = strlen(client_input.dir);
		size_t urlfilelen = strlen(client_input.url.file) + 1;
		char path[dirlen + strlen("/") + urlfilelen];
		strcpy(path, client_input.dir);
		strcat(path, "/");
		strcat(path, client_input.url.file);
		out_file = fopen(path, "w");
	}
	if (out_file == NULL)
	{
		fprintf(stderr, "[%s] ERROR: Opening file %s failed\n", program_name, client_input.file);
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief connects to the socket with the given client_input information and safes to socket_file
 *
 * @param client_input
 */
static void connect_socket_file(client_input client_input)
{
	struct addrinfo hints, *ai;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int ai_err;
	if ((ai_err = getaddrinfo(client_input.url.hostname, client_input.port, &hints, &ai)) != 0)
	{
		exit_with_error("Couldn't get address information!");
	}
	int sockfd;
	if ((sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1)
	{
		freeaddrinfo(ai);
		exit_with_error("Couldn't create socket!");
	}
	if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
	{
		exit_with_error("Couldn't connect to socket!");
	}
	freeaddrinfo(ai);

	if ((socket_file = fdopen(sockfd, "r+")) == NULL)
	{
		exit_with_error("Opening socket file failed!");
	}
}

/**
 * @brief sends a request for a file to the server and handles the response if the reponse header is
 * valid the header is skipped and stops where the real content starts
 *
 * @param client_url
 */
static void send_receive(client_url client_url)
{
	if (fprintf(socket_file, "GET /%s HTTP/1.1\r\nhostname: %s\r\nConnection: close\r\n\r\n", client_url.dir, client_url.hostname) < 0)
	{
		exit_with_error("Couldn't write request to socket!");
	}

	if (fflush(socket_file) == EOF)
	{
		exit_with_error("Failed to flush request to socket!");
	}

	char *line_buffer = NULL;
	size_t line_buffer_size = 0;

	if (getline(&line_buffer, &line_buffer_size, socket_file) < 0)
	{
		fprintf(stderr, "[%s] ERROR: Couldn't read response header!\n", program_name);
		free(line_buffer);
		exit(2);
	}

	char *protocol_version = strtok(line_buffer, " ");
	char *status_code = strtok(NULL, " ");
	char *status_description = strtok(NULL, "\r\n");

	if (protocol_version == NULL || status_code == NULL || status_description == NULL)
	{
		fprintf(stderr, "[%s] ERROR: Protocol Error!\n", program_name);
		free(line_buffer);
		exit(2);
	}

	if (strcmp(protocol_version, "HTTP/1.1") != 0 || (strtol(status_code, NULL, 10) == 0 && strcmp(status_code, "0") != 0))
	{
		fprintf(stderr, "[%s] ERROR: Protocol Error!\n", program_name);
		free(line_buffer);
		exit(2);
	}

	if (strcmp(status_code, "200") != 0)
	{
		fprintf(stderr, "ERROR: %s %s %s!\n", program_name, status_code, status_description);
		free(line_buffer);
		exit(3);
	}

	while (getline(&line_buffer, &line_buffer_size, socket_file) != -1)
	{
		if (strcmp(line_buffer, "\r\n") == 0)
			break;
	}
	free(line_buffer);
}

/**
 * @brief reads from the socket_file and writes to the out_file
 *
 */
static void read_write_response(void)
{
	char buf[1024];
	while (fgets(buf, sizeof(buf), socket_file) != NULL)
	{
		fputs(buf, out_file);
	}
}

/**
 * @brief main function calls clean_up when exited, sets the program_name
 * parses the input, if the input is correct the url is parsed
 * opens the file, connects to the socket, send and receive and at the end read and write the response
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[])
{
	program_name = argv[0];

	if (atexit(clean_up) < 0)
	{
		exit_with_error("Setting up clean_up function didn't work!");
	}

	client_input client_input;
	client_input.port = "80";
	bool port_set = false;
	client_input.file = NULL, client_input.dir = NULL;
	int opt;
	while ((opt = getopt(argc, argv, "p:o:d:")) != -1)
	{
		switch (opt)
		{
		case 'p':
			if (!port_set)
			{
				char *ptr;
				strtol(optarg, &ptr, 10);
				if (ptr != '\0')
				{
					client_input.port = optarg;
					port_set = true;
					break;
				}
			}
			wrong_input("-p can only be set once");
		case 'o':
			if (client_input.file == NULL)
			{
				client_input.file = optarg;
				check_execution(client_input.file, client_input.dir);
				break;
			}
			wrong_input("-o can only be set once");
		case 'd':
			if (client_input.dir == NULL)
			{
				client_input.dir = optarg;
				check_execution(client_input.file, client_input.dir);
				break;
			}
			wrong_input("-d can only be set once");
		case '?':
			usage();
		default:
			assert(false);
		}
	}

	// Get the URL
	if (optind != argc - 1)
	{
		usage();
	}

	// parse the URL
	client_input.url = parse_url(argv[optind]);

	open_file(client_input);
	connect_socket_file(client_input);
	send_receive(client_input.url);
	read_write_response();

	exit(EXIT_SUCCESS);
}