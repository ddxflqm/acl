#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

typedef struct EVENT EVENT;

struct EVENT {
	int  setsize;

	const char *(*name)(void);
	int  (*loop)(EVENT *, struct timeval *);
	int  (*add)(EVENT *, int, int);
	void (*del)(EVENT *, int, int);
	void (*free)(EVENT *);
};

#endif
