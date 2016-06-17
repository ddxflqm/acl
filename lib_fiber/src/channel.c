/* Copyright (c) 2005 Russ Cox, MIT; see COPYRIGHT */

#include "stdafx.h"
#include "fiber.h"

CHAN* chan_create(int elemsize, int bufsize)
{
	CHAN *c;

	c = (CHAN *) acl_mycalloc(1, sizeof(*c) + bufsize * elemsize);
	c->elemsize = elemsize;
	c->bufsize  = bufsize;
	c->nbuf     = 0;
	c->buf      = (unsigned char *)(c + 1);
	return c;
}

/* bug - work out races */
void chan_free(CHAN *c)
{
	if(c != NULL) {
		acl_myfree(c->name);
		acl_myfree(c->arecv.a);
		acl_myfree(c->asend.a);
		acl_myfree(c);
	}
}

static void array_add(FIBER_ALT_ARRAY *a, FIBER_ALT *alt)
{
	if (a->n == a->m) {
		a->m += 16;
		a->a = acl_myrealloc(a->a, a->m * sizeof(a->a[0]));
	}

	a->a[a->n++] = alt;
}

static void array_del(FIBER_ALT_ARRAY *a, int i)
{
	--a->n;
	a->a[i] = a->a[a->n];
}

/*
 * doesn't really work for things other than CHANSND and CHANRCV
 * but is only used as arg to chan_array, which can handle it
 */
#define otherop(op)	(CHANSND+CHANRCV-(op))

static FIBER_ALT_ARRAY* chan_array(CHAN *c, unsigned int op)
{
	switch(op){
	case CHANSND:
		return &c->asend;
	case CHANRCV:
		return &c->arecv;
	default:
		return NULL;
	}
}

static int alt_can_exec(FIBER_ALT *a)
{
	FIBER_ALT_ARRAY *ar;
	CHAN *c;

	if (a->op == CHANNOP)
		return 0;
	c = a->c;
	if (c->bufsize == 0) {
		ar = chan_array(c, otherop(a->op));
		return ar && ar->n;
	}

	switch(a->op){
		default:
			return 0;
		case CHANSND:
			return c->nbuf < c->bufsize;
		case CHANRCV:
			return c->nbuf > 0;
	}
}

static void alt_queue(FIBER_ALT *a)
{
	FIBER_ALT_ARRAY *ar;

	ar = chan_array(a->c, a->op);
	array_add(ar, a);
}

static void alt_dequeue(FIBER_ALT *a)
{
	FIBER_ALT_ARRAY *ar;
	unsigned int i;

	ar = chan_array(a->c, a->op);
	if (ar == NULL){
		fprintf(stderr, "bad use of altdequeue op=%d\n", a->op);
		abort();
	}

	for (i = 0; i < ar->n; i++) {
		if (ar->a[i] == a) {
			array_del(ar, i);
			return;
		}
	}

	fprintf(stderr, "cannot find self in altdq\n");
	abort();
}

static void alt_all_dequeue(FIBER_ALT *a)
{
	int i;

	for ( i = 0; a[i].op != CHANEND && a[i].op != CHANNOBLK; i++) {
		if (a[i].op != CHANNOP)
			alt_dequeue(&a[i]);
	}
}

static void amove(void *dst, void *src, unsigned int n)
{
	if (dst) {
		if (src == NULL)
			memset(dst, 0, n);
		else
			memmove(dst, src, n);
	}
}

/*
 * Actually move the data around.  There are up to three
 * players: the sender, the receiver, and the channel itself.
 * If the channel is unbuffered or the buffer is empty,
 * data goes from sender to receiver.  If the channel is full,
 * the receiver removes some from the channel and the sender
 * gets to put some in.
 */
static void alt_copy(FIBER_ALT *s, FIBER_ALT *r)
{
	FIBER_ALT *t;
	CHAN *c;
	unsigned char *cp;

	/*
	 * Work out who is sender and who is receiver
	 */
	if (s == NULL && r == NULL)
		return;
	assert(s != NULL);
	c = s->c;
	if (s->op == CHANRCV) {
		t = s;
		s = r;
		r = t;
	}
	assert(s==NULL || s->op == CHANSND);
	assert(r==NULL || r->op == CHANRCV);

	/*
	 * CHAN is empty (or unbuffered) - copy directly.
	 */
	if (s && r && c->nbuf == 0) {
		amove(r->v, s->v, c->elemsize);
		return;
	}

	/*
	 * Otherwise it's always okay to receive and then send.
	 */
	if (r) {
		cp = c->buf + c->off*c->elemsize;
		amove(r->v, cp, c->elemsize);
		--c->nbuf;
		if (++c->off == c->bufsize)
			c->off = 0;
	}

	if (s) {
		cp = c->buf + (c->off + c->nbuf) % c->bufsize * c->elemsize;
		amove(cp, s->v, c->elemsize);
		++c->nbuf;
	}
}

static void alt_exec(FIBER_ALT *a)
{
	FIBER_ALT_ARRAY *ar;
	FIBER_ALT *other;
	CHAN *c;
	int i;

	c = a->c;
	ar = chan_array(c, otherop(a->op));

	if (ar && ar->n) {
		i = rand() % ar->n;
		other = ar->a[i];
		alt_copy(a, other);
		alt_all_dequeue(other->xalt);
		other->xalt[0].xalt = other;

		fiber_ready(other->fiber);
	 } else
		alt_copy(a, NULL);
}

#define dbgalt 0

static int chan_alt(FIBER_ALT *a)
{
	int i, j, ncan, n, canblock;
	CHAN *c;
	FIBER *t;

	for (i = 0; a[i].op != CHANEND && a[i].op != CHANNOBLK; i++) {}

	n = i;
	canblock = a[i].op == CHANEND;

	t = fiber_running();

	for (i = 0; i < n; i++) {
		a[i].fiber = t;
		a[i].xalt = a;
	}

	if (dbgalt)
		printf("alt ");

	ncan = 0;

	for (i = 0; i < n; i++) {
		c = a[i].c;

		if (dbgalt)
			printf(" %c:", "esrnb"[a[i].op]);
		if (dbgalt) {
			if (c->name)
				printf("%s", c->name);
			else
				printf("%p", c);
		}

		if (alt_can_exec(&a[i])) {
			if (dbgalt)
				printf("*");
			ncan++;
		}
	}

	if (ncan) {
		j = rand() % ncan;

		for (i = 0; i < n; i++) {
			if (!alt_can_exec(&a[i]))
				continue;

			if (j-- > 0)
				continue;

			if (dbgalt) {
				c = a[i].c;
				printf(" => %c:", "esrnb"[a[i].op]);
				if(c->name)
					printf("%s", c->name);
				else
					printf("%p", c);
				printf("\n");
			}

			alt_exec(&a[i]);
			return i;
		}
	}

	if (dbgalt)
		printf("\n");

	if (!canblock)
		return -1;

	for (i = 0; i < n; i++) {
		if (a[i].op != CHANNOP)
			alt_queue(&a[i]);
	}

	fiber_switch();

	/*
	 * the guy who ran the op took care of dequeueing us
	 * and then set a[0].alt to the one that was executed.
	 */
	return a[0].xalt - a;
}

static int chan_op(CHAN *c, int op, void *p, int canblock)
{
	FIBER_ALT a[2];

	a[0].c = c;
	a[0].op = op;
	a[0].v = p;
	a[1].op = canblock ? CHANEND : CHANNOBLK;

	if (chan_alt(a) < 0)
		return -1;
	return 1;
}

int chan_send(CHAN *c, void *v)
{
	return chan_op(c, CHANSND, v, 1);
}

int chan_send_nb(CHAN *c, void *v)
{
	return chan_op(c, CHANSND, v, 0);
}

int chan_recv(CHAN *c, void *v)
{
	return chan_op(c, CHANRCV, v, 1);
}

int chan_recv_nb(CHAN *c, void *v)
{
	return chan_op(c, CHANRCV, v, 0);
}

int chan_sendp(CHAN *c, void *v)
{
	return chan_op(c, CHANSND, (void *) &v, 1);
}

void *chan_recvp(CHAN *c)
{
	void *v;

	chan_op(c, CHANRCV, (void *) &v, 1);
	return v;
}

int chan_sendp_nb(CHAN *c, void *v)
{
	return chan_op(c, CHANSND, (void *) &v, 0);
}

void *chan_recvp_nb(CHAN *c)
{
	void *v;

	chan_op(c, CHANRCV, (void *) &v, 0);
	return v;
}

int chan_sendul(CHAN *c, ulong val)
{
	return chan_op(c, CHANSND, &val, 1);
}

unsigned long chan_recvul(CHAN *c)
{
	unsigned long val;

	chan_op(c, CHANRCV, &val, 1);
	return val;
}

int chan_sendul_nb(CHAN *c, ulong val)
{
	return chan_op(c, CHANSND, &val, 0);
}

unsigned long chan_recvul_nb(CHAN *c)
{
	unsigned long val;

	chan_op(c, CHANRCV, &val, 0);
	return val;
}
