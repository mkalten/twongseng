#include "twongseng.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

static bool running = false;
static bool verbose = false;
static std::string host("localhost");
static int port = 3333;
static int device = 0;

static void show_help()
{
	std::cout << "Usage: twongseng -t [transport] -p [port] -d [device]" << std::endl;
	std::cout << "        -t [hostname] for remote TUIO/UDP client" << std::endl;
	std::cout << "        -t TCP for TUIO/TCP" << std::endl;
	std::cout << "        -t WEB for TUIO/WEB" << std::endl;
	std::cout << "        -p [port] for alternative port number" << std::endl;
	std::cout << "        -l list available devices" << std::endl;
	std::cout << "        -v verbose output" << std::endl;
	std::cout << "        -h show this help" << std::endl;
}

static void stop(int param)
{
	running = false;
}

static void init(int argc, char** argv)
{
	char c;

	while ((c = getopt(argc, argv, "t:p:d:lvh")) != -1) {
		switch (c) {
			case 't':
				host = std::string(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'd':
				device = atoi(optarg);
				break;
			case 'v':
				verbose = true;
				break;
			case 'l':
				twongseng_list_devices();
				exit(0);
			case 'h':
				show_help();
				exit(0);
			/*case '?':
				if (optopt == 'n')
					fprintf (stderr, "Option -n requires a host name.\n");
				if (optopt == 'p')
					fprintf (stderr, "Option -p requires a port number.\n");
				if (optopt == 'd')
					fprintf (stderr, "Option -d requires a device number.\n");
				else if (isprint (optopt)) fprintf (stderr, "Unknown option '-%c'.\n", optopt);
				else fprintf (stderr, "Unknown option character '\\x%x'.\n", optopt);*/
			default:
				show_help();
				exit(1);
		}
	}
}

int main(int argc, char** argv)
{
	init(argc, argv);
	std::cout << "Host: " << host << std::endl;
	std::cout << "Port: " << port << std::endl;
	std::cout << "Device: " << device << std::endl;
	std::cout << "Verbose: " << verbose << std::endl;
	std::cout << "Press Ctrl+C to end this program." << std::endl;

	signal(SIGINT, stop);
	signal(SIGHUP, stop);
	signal(SIGQUIT, stop);
	signal(SIGTERM, stop);

	twongseng_set_hostname_and_port(host.c_str(), port);
	if ((host=="TCP") || (host=="tcp")) twongseng_set_protocol(TUIO_TCP);
	else if ((host=="WEB") || (host=="web")) twongseng_set_protocol(TUIO_WEB);
	twongseng_set_verbose(verbose);
	twongseng_set_device(device);
	twongseng_start();

	// Loop until the program is stopped.
	running = true;
	while (running) { 
		usleep(1000);
	};

	std::cout << "Cleaning up.." << std::endl;

	twongseng_stop();

	std::cout << "Program stopped." << std::endl;

	return 0;
}


