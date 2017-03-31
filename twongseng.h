#ifndef TWONGSENG_H_
#define TWONGSENG_H_

#ifdef __cplusplus
extern "C" {
#endif
	
#define TUIO_UDP 0
#define TUIO_TCP 1
#define TUIO_WEB 2

void twongseng_set_protocol(int);
void twongseng_set_hostname_and_port(const char*, int);
void twongseng_set_device(int);
void twongseng_set_verbose(int);
void twongseng_start();
void twongseng_stop();
void twongseng_list_devices();

#ifdef __cplusplus
}
#endif

#endif
