/**
 * @file server.c
 * @author Maximilian Gaber 52009273
 * @brief HTTP server that waits for connections and sends the requested files
 * @version 0.1
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>
#include <getopt.h>
#include <assert.h>

static char *program_name = "<not set>";
static volatile sig_atomic_t quit = 0;
static int sockfd;

typedef struct server_input
{
	char *port;
	char *index;
	char *doc_root;
} server_input;

typedef struct server_response
{
	char *code;
	char *description;
	FILE *request_file;
} server_response;

/**
 * @brief is called when signal is detected
 * 
 * @param signal 
 */
static void handle_signal(int signal)
{
	quit = 1;
}

/**
 * @brief listen to signal from user
 * 
 */
static void listen_signal(void)
{
	struct sigaction sa = {.sa_handler = handle_signal};
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

/**
 * @brief prints the right usage of the program
 * 
 */
static void usage(void)
{
	fprintf(stderr, "Usage: %s [-p PORT] [-i INDEX] DOC_ROOT\n", program_name);
	fprintf(stderr, "EXAMPLE: %s -p 1280 -i index.html ˜/Documents/my_website/\n", program_name);
	exit(EXIT_FAILURE);
}

/**
 * @brief exits the program with error and a specific error message
 * 
 * @param message 
 */
static void exit_with_error(char *message)
{
	fprintf(stderr, "[%s] ERROR: %s\n", program_name, message);
	exit(EXIT_FAILURE);
}

/**
 * @brief checks if directory exists
 * 
 * @param server_input 
 */
static void check_dir(server_input server_input)
{
	DIR *dir = opendir(server_input.doc_root);
	if (dir == NULL)
	{
		exit_with_error("Couldn't open directory!");
	}
	closedir(dir);
}

/**
 * @brief checks if the path is right only when index 
 * 
 * @param server_input 
 */
static void check_path(server_input server_input)
{
	size_t doc_root_len = strlen(server_input.doc_root);
	size_t index_len = strlen(server_input.index) + 1;
	char path[doc_root_len + strlen("/") + index_len];
	strcpy(path, server_input.doc_root);
	strcat(path, "/");
	strcat(path, server_input.index);
	FILE *file = fopen(path, "r");
	if (file == NULL)
	{
		exit_with_error("Couldn't open file!");
	}
}


/**
 * @brief starts the socket and returns the socket file directory
 * 
 * @param server_input 
 * @return int 
 */
static int start_socket(server_input server_input)
{
	struct addrinfo hints, *ai;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	int res = getaddrinfo(NULL, server_input.port, &hints, &ai);
	if (res != 0)
	{
		exit_with_error("Couldn't get adress information!");
	}
	int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sockfd < 0)
	{
		exit_with_error("Couldn't create socket!");
	}
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
	{
		exit_with_error("Couldn't bind socket!");
	}
	freeaddrinfo(ai);
	return sockfd;
}

/**
 * @brief handles all incoming requests
 * 
 * @param socket_file 
 * @param server_input 
 * @return server_response 
 */
static server_response handle_request(FILE *socket_file, server_input server_input)
{
	server_response server_response;
	server_response.code = "500";
	server_response.description = "Internal Server Error";
	char *line_buffer = NULL;
	size_t line_buffer_size = 0;
	if (getline(&line_buffer, &line_buffer_size, socket_file) == -1)
	{
		fprintf(stderr, "[%s] ERROR: Couldn't read request header!\n", program_name);
		while ((getline(&line_buffer, &line_buffer_size, socket_file)) != -1)
		{
			if (strcmp(line_buffer, "\r\n") == 0)
				break;
		}
		free(line_buffer);
		return server_response;
	}

	char *request_method = strtok(line_buffer, " ");
	char *request_resource = strtok(NULL, " ");
	char *protocol_version = strtok(NULL, "\r\n");

	// 400 Bad Request
	if (request_method == NULL || request_resource == NULL || protocol_version == NULL || strcmp(protocol_version, "HTTP/1.1") != 0)
	{
		server_response.code = "400";
		server_response.description = "Bad request";
		while ((getline(&line_buffer, &line_buffer_size, socket_file)) != -1)
		{
			if (strcmp(line_buffer, "\r\n") == 0)
				break;
		}
		free(line_buffer);
		return server_response;
	}

	// 501 NOT implemented
	if (strcmp(request_method, "GET") != 0)
	{
		server_response.code = "501";
		server_response.description = "Not implemented";
		while ((getline(&line_buffer, &line_buffer_size, socket_file)) != -1)
		{
			if (strcmp(line_buffer, "\r\n") == 0)
				break;
		}
		free(line_buffer);
		return server_response;
	}

	char path[strlen(server_input.doc_root) + strlen(server_input.index) + strlen(request_resource)];
	strcpy(path, server_input.doc_root);
	strcat(path, "/");
	if (strcmp(request_resource, "/") == 0)
	{
		strcat(path, server_input.index);
	}
	else
	{
		strcat(path, request_resource);
	}

	// Find the requested file
	FILE *request_file = fopen(path, "r");
	if (request_file == NULL)
	{
		server_response.code = "404";
		server_response.description = "Not found";
		while ((getline(&line_buffer, &line_buffer_size, socket_file)) != -1)
		{
			if (strcmp(line_buffer, "\r\n") == 0)
				break;
		}
		free(line_buffer);
		return server_response;
	}

	// Read request to end
	while ((getline(&line_buffer, &line_buffer_size, socket_file)) != -1)
	{
		if (strcmp(line_buffer, "\r\n") == 0)
			break;
	}

	free(line_buffer);
	server_response.code = "200";
	server_response.description = "OK";
	server_response.request_file = request_file;
	return server_response;
}

/**
 * @brief reads from the requested file and writes into the socket file
 * 
 * @param server_response 
 * @param socket_file 
 */
static void read_write_response(server_response server_response, FILE *socket_file)
{
	char buffer[1024];
	while (fgets(buffer, sizeof(buffer), server_response.request_file) != NULL)
	{
		fputs(buffer, socket_file);
	}
}

/**
 * @brief Gets the file size 
 * 
 * @param file 
 * @return size_t 
 */
static size_t get_file_size(FILE *file)
{
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

/**
 * @brief writes message on success = 200
 * 
 * @param socket_file 
 * @param server_response 
 */
static void write_success(FILE *socket_file, server_response server_response)
{
	char time_text[128];
	time_t t = time(NULL);
	struct tm *tmp;
	tmp = gmtime(&t);
	strftime(time_text, sizeof(time_text), "%a, %d %b %y %T %Z", tmp);

	fprintf(socket_file, "HTTP/1.1 %s %s\r\nDate: %s\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n", server_response.code,
			server_response.description, time_text, get_file_size(server_response.request_file));
	read_write_response(server_response, socket_file);
}


/**
 * @brief flushes the code and description on error to the socket file
 * 
 * @param socket_file 
 * @param server_response 
 */
static void write_error(FILE *socket_file, server_response server_response){
	fprintf(socket_file, "HTTP/1.1 %s (%s)\r\nConnection: close\r\n\r\n", server_response.code, server_response.description);
	fflush(socket_file);
}

/**
 * @brief listens to all connections until done 
 * 
 * @param sockfd 
 * @param server_input 
 */
static void listen_conncetions(int sockfd, server_input server_input)
{
	if (listen(sockfd, 16) == -1)
	{
		exit_with_error("Couldn't listen to the socket!");
	}
	while (quit != 1)
	{
		// Wait for a request and receive the socket
		int connfd = accept(sockfd, NULL, NULL);
		if (connfd == -1)
		{
			if (errno == EINTR)
			{
				fprintf(stderr, "[%s] ERROR: Couldn't connect: %s\n", program_name, strerror(errno));
			}
			continue;
		}
		FILE *socket_file = fdopen(connfd, "r+");
		if (socket_file == NULL)
		{
			fprintf(stderr, "[%s] ERROR: Couldn't open socket file: %s\n", program_name, strerror(errno));
			close(connfd);
			continue;
		}
		// handle request
		server_response server_response = handle_request(socket_file, server_input);
		
		if (strcmp(server_response.code, "200") == 0)
		{
			write_success(socket_file, server_response);
		}
		else
		{
			write_error(socket_file, server_response);
		}
		if (socket_file != NULL)
		{
			fclose(socket_file);
		}
		if (server_response.request_file != NULL)
		{
			fclose(server_response.request_file);
		}
	}
}

/**
 * @brief cleans up when program exited
 * 
 */
static void clean_up(void)
{
	close(sockfd);
}

/**
 * @brief convert input, check diretory, check path if index is given, starts to listen to signals
 * starts the socket and starts to listen to connections
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
	program_name = argv[0];
	int sockfd;
	if (atexit(clean_up) < 0)
	{
		exit_with_error("Setting up clean_up function didn't work!");
	}
	server_input server_input;
	server_input.port = "8080";
	server_input.index = "index.html";
	bool port_set = false;
	bool index_set = false;
	int opt;
	while ((opt = getopt(argc, argv, "p:i:")) != -1)
	{
		switch (opt)
		{
		case 'p':
			if (port_set == true)
			{
				fprintf(stderr, "[%s] ERROR: Invalid options!", program_name);
				usage();
			}
			server_input.port = optarg;
			port_set = true;
			break;
		case 'i':
			if (index_set == true)
			{
				fprintf(stderr, "[%s] ERROR: Invalid options!", program_name);
				usage();
			}
			server_input.index = optarg;
			index_set = true;
			break;
		case '?':
			usage();
		default:
			assert(false);
		}
	}
	if (optind != argc - 1)
	{
		usage();
	}
	server_input.doc_root = argv[optind];
	check_dir(server_input);
	if (index_set)
	{
		check_path(server_input);
	}
	listen_signal();
	sockfd = start_socket(server_input);
	listen_conncetions(sockfd, server_input);
	exit(EXIT_SUCCESS);
}