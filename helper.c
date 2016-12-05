#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "main.h"

#define UNIX_SOCKET "/tmp/usock"

int sk;

#define P_START 0xaabbccdd
#define P_END	0x11223344


float (*funcs[])(card_t *, combination_t *, int, fi_t *) = {
	get_flop_prob,
	get_turn_prob,
	get_river_prob
};

typedef struct ipk_s {
	unsigned int start;

	unsigned int state;
	card_t cards[7];

	unsigned int end;
} __attribute__((packed)) ipk_t;


typedef struct opk_s {
	unsigned int start;

	uint8_t draw;
	combination_t combination;
	float prob;

	unsigned int end;
} __attribute__((packed)) opk_t;

void signal_handler(int sig) {
	close(sk);
	unlink(UNIX_SOCKET);
	
	signal(sig, SIG_DFL);
	raise(sig);
}

void my_daemon(void) {
	pid_t pid;
	int i;

	pid = fork();
	if (pid < 0)
		exit(1);
	else if (pid)
		exit(0);

	if (setsid() < 0)
		exit(1);

	signal(SIGHUP, SIG_IGN);
//	signal(SIGINT, SIG_IGN);

	pid = fork();
	if (pid < 0)
		exit(1);
	else if (pid)
		exit(0);

	umask(0);
	if (!chdir("/"))
		exit(0);

	for (i = 0; i < 1024; i++)
		close(i);

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);
}

int inline get_cards_count(int state) {
	switch (state) {
		case ST_PREFLOP:
			return 2;
		case ST_FLOP:
			return 5;
		case ST_TURN:
			return 6;
		case ST_RIVER:
			return 7;
	}

	return 0;
}

void helper_loop(void) {
	struct sockaddr_un un;
	uint8_t snum;
	ssize_t count;
	ipk_t ipkt;
	opk_t opkt;
	int i, total_cards, csk;
	socklen_t socket_length;
	struct sockaddr_un cun;
	unsigned char *p;
	fi_t fi;
	float prob;
	float (*func)(card_t *, combination_t *, int, fi_t *);
	combination_t combination, alone_combination;
	card_t cards[10], sc;
	int sdraw, fdraw, pair_state, pcount;

	unlink(UNIX_SOCKET);
//	my_daemon();

	sk = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sk < 0) 
		exit(1);


	memset(&un, 0, sizeof(struct sockaddr));
	un.sun_family = AF_UNIX;
	memcpy(un.sun_path, UNIX_SOCKET, strlen(UNIX_SOCKET));
	if (bind(sk, (struct sockaddr *) &un, sizeof(struct sockaddr)) < 0)
		exit(1);

	listen(sk, 1);

	socket_length = sizeof(struct sockaddr);
	csk = accept(sk, (struct sockaddr *)&cun, &socket_length);
	if (csk < 0) {
		printf("accept error %s\n",strerror(errno));
		exit(1);
	}

	setvbuf(stdout, NULL,  _IONBF,0);
	//printf("send!\n");
	while (1) {

		gc.cnt = 0;
		pthread_mutex_init(&gc.mtx, NULL);

		memset(&ipkt, 0, sizeof(ipkt));
		memset(cards, 0, sizeof(card_t) * 8);
		memset(&fi, 0, sizeof(fi));
		count = recv(csk, &ipkt, sizeof(ipkt), 0);
		if (count < 0)
			continue;

		if (!count) {
			printf("bingo!\n");
			exit(0);
		}

		if (ipkt.start != P_START) 
			continue;

		total_cards = get_cards_count(ipkt.state);

		p = (unsigned char *) &ipkt.cards[0];
		for (i = 0; i < total_cards; i++) 
			p += sizeof(card_t);
		
		ipkt.end = *(unsigned int *) p;
		if (ipkt.end != P_END)
			continue;


		opkt.start = ipkt.start;
		opkt.end   = ipkt.end;

		if (ipkt.state > ST_RIVER)
			continue;

/*		printf("c=%x %x %x (%x-%x)\n",ipkt.start, ipkt.state, ipkt.end, P_START, P_END);

*/
	//	for (i=0; i<total_cards;i++){
	//		printf("(%x %x) ", ipkt.cards[i].num, ipkt.cards[i].suit);
	//		print_card(card_idx(&ipkt.cards[i]));
	//	}	

		pcount = 0;
		memcpy(cards, ipkt.cards, sizeof(card_t) * total_cards);
		switch (ipkt.state) {
			case ST_PREFLOP:
				prob = get_pf_probs(cards, 2, &fi);	
				break;
			case ST_FLOP:
				prob = get_flop_prob(cards, &combination, 2, &fi);

			//	print_item(0,0,0,&combination);


			//	printf("ccc=%u\n",combination.type);

				memset(&alone_combination, 0, sizeof(combination_t));
				set_combination(cards + 2, &alone_combination, 3);
			//	print_item(0,0,0,&alone_combination);
				sdraw = _check_straight_draw(cards, 5);
				fdraw = _check_flush_draw(cards, 5);
				pcount = 5;
				
				break;
			case ST_TURN:
			case ST_RIVER:
				cards[TURN_CARD].num = 0;
				prob = get_flop_prob(cards, &combination, 2, &fi);

				memset(cards, 0, sizeof(card_t) * 8);
				memcpy(cards, ipkt.cards, sizeof(card_t) * total_cards);

				func = funcs[ipkt.state - 1];	
				prob = func(cards, &combination, 2, &fi);


				memset(&alone_combination, 0, sizeof(combination_t));
				if (ipkt.state == ST_TURN) {
					set_combination(cards + 2, &alone_combination, 4);
	//				print_item(0,0,0,&alone_combination);
					sdraw = _check_straight_draw(cards, 6);
					fdraw = _check_flush_draw(cards, 6);
					pcount = 6;
				} else {
					get_flop_prob(cards + 2, &alone_combination, 2, &fi);
	//				print_item(0,0,0,&combination);
					sdraw = fdraw = 0;

					pcount = 7;
	//				printf("at=%u\n",alone_combination.type);
	//				print_item(0,0,0,&alone_combination);
				}

				

	//			printf("da! -> %u\n", combination.type);

				break;
			
		}

	//	printf("ct0=%u %u\n",combination.type, alone_combination.type);
	//	if (comb_compare(&combination, &alone_combination) <= 0) {
		if (combination.type <= alone_combination.type) {
			combination.type = COMB_NONE;
		}

		if (combination.type >= COMB_STRAIGHT)
			sdraw = 0;

		if (combination.type >= COMB_FLUSH)
			fdraw = 0;

		if (combination.type == COMB_DOUBLE_PAIR &&
			alone_combination.type == COMB_SINGLE_PAIR) {
			combination.type = COMB_SINGLE_PAIR;
			if (combination.high_card == alone_combination.high_card) {
				combination.high_card = combination.low_card;
			}
			combination.low_card = 0;
		}

		pair_state = 0;
		if (combination.type == COMB_SINGLE_PAIR && ipkt.state != ST_PREFLOP) {
			pair_state = _pair_state(cards, pcount, combination.high_card);
		}

	//	printf("ps=%u %u\n",pair_state, sdraw);

	//	printf("ct=%u draw=%u\n",combination.type,sdraw);
		opkt.prob = prob;
		opkt.draw = sdraw | fdraw | pair_state;
		memcpy(&opkt.combination, &combination, sizeof(combination_t));

		send(csk, &opkt, sizeof(opkt), 0);

//		printf("\n");

		free_gc();
			
	}
}
