#ifndef _PK_H
#define _PK_H

#include <pthread.h>

// spades, clubs, diamonds, hearts
enum {
	SUIT_PIKI,
	SUIT_TREFI,
	SUIT_BUBI,
	SUIT_CHERVI
};

#define SUIT_ANY (SUIT_CHERVI + 1)

enum {
	CARD_JACK 	= 11,
	CARD_QUEEN 	= 12,
	CARD_KING 	= 13,
	CARD_ACE 	= 14
};

enum {
	COMB_NONE,
	COMB_SINGLE_PAIR,
	COMB_DOUBLE_PAIR,
	COMB_TRIPLET,
	COMB_STRAIGHT,
	COMB_FLUSH,
	COMB_FULL_HOUSE,
	COMB_CARE,
	COMB_STRAIGHT_FLUSH,
	COMB_ROYAL_FLUSH
};

#define MAX_COMB COMB_ROYAL_FLUSH

static char *combs[10] = {
	"none",
	"pair",
	"doublepair",
	"triplet",
	"straight",
	"flush",
	"fullhouse",
	"care",
	"straightflush",
	"royalflush"
};

enum {
	MY_CARD1,
	MY_CARD2,
	FLOP_CARD1,
	FLOP_CARD2,
	FLOP_CARD3,
	TURN_CARD,
	RIVER_CARD
};

static unsigned char *v_suits[4] = {
	"\xe2\x99\xa0",
	"\xe2\x99\xa3",
	"\xe2\x99\xa6",
	"\xe2\x99\xa5"
};


#define	TR_COMB_POSSIBLE	0x0
#define	TR_COMB_IMPOSSIBLE	0x1


typedef struct card_s {
	uint8_t num;
	uint8_t suit;
} card_t;

#define TOTAL_CARDS	52
#define SUIT_CARDS	13

struct cnts_s {
	uint8_t num;
	uint8_t neigh_count;
	uint8_t neigh[TOTAL_CARDS];
};


#define MATRIX_WHOLE	255


#define MYCARDS_CONTEXT		0x0
#define OPPCARDS_CONTEXT	0x1


typedef struct combination_s {
	uint8_t type;
	uint8_t high_card, low_card;
	uint8_t kicker[3];
} combination_t;

static void display_tr(combination_t *(*)[TOTAL_CARDS]);
static void init_tr(combination_t *(*)[TOTAL_CARDS], card_t *);

typedef struct fold_info_s {
	uint32_t cc, drawcc;
	int folders;
	float decprob, draw;
} fi_t;

static inline uint8_t card_idx(card_t *card) {
	return card->suit * SUIT_CARDS + (card->num - 2);
}

static inline uint8_t card_num(uint8_t idx) {
	return idx % SUIT_CARDS + 2;
}

static inline uint8_t card_suit(uint8_t idx) {
	return (uint8_t) idx / SUIT_CARDS;
}

static inline void simple_sort(uint8_t *array, int count) {
	uint8_t helper[CARD_ACE + 1];
	int i, j, idx;

	for (i = 0; i < CARD_ACE + 1; i++)
		helper[i] = 0;

	for (i = 0; i < count; i++) {
		helper[array[i]]++;
	}
	

	idx = 0;
	for (i = 0; i < CARD_ACE + 1; i++) {
		for (j = 0; j < helper[i]; j++) {
			array[idx++] = i;	
		}
	}

}

#define MAX_COLLECTED_COMBS 10000000

typedef struct gc_s {
	void *pointer[MAX_COLLECTED_COMBS];
	pthread_mutex_t mtx;
	uint32_t cnt;
} gc_t;

extern gc_t gc;
void free_gc(void);

static inline combination_t *get_combination(uint8_t type, uint8_t high_card) {
	combination_t *combination;

	combination = malloc(sizeof(combination_t *));
	if (!combination) {
		fprintf(stderr, "Can't alloc memory\n");
		exit(1);
	}

	combination->type = type;
	combination->high_card = high_card;
	combination->low_card = 0;
	combination->kicker[0] = combination->kicker[1] = 0;
	combination->kicker[2] = 0;

	pthread_mutex_lock(&gc.mtx);
	gc.pointer[gc.cnt++] = (void *) combination;
	pthread_mutex_unlock(&gc.mtx);
	return combination;
}


static inline void matrix_value(void *m) {
	combination_t *combination = (combination_t *) m;

	if (m == (void *) TR_COMB_IMPOSSIBLE) {
		printf("x");
		return;
	}
	else if (m == (void *) TR_COMB_POSSIBLE) {
		printf("0");
		return;
	}

	printf("%d-%d ", combination->type, combination->high_card);
}

typedef struct care_data_s {
	uint8_t cnt, num;
	uint8_t suit[3];
} care_data_t;

uint16_t set_matrix(uint8_t, uint8_t, combination_t *, combination_t *(*)[TOTAL_CARDS]);
uint16_t set_matrix_line(uint8_t, combination_t *, combination_t *(*)[TOTAL_CARDS]);
uint16_t set_matrix_square(combination_t *, combination_t *(*)[TOTAL_CARDS]);
uint16_t set_matrix_single(uint8_t, uint8_t, combination_t *, combination_t *(*)[TOTAL_CARDS]);
uint16_t save_straight(uint8_t, uint8_t, uint8_t, uint8_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);

/* combination count */
uint16_t royal_flush_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t straight_flush_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t care_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t full_house_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t flush_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t straight_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t triplet_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t double_pair_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t single_pair_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint16_t none_cc(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS]);

uint16_t straight_common(uint8_t *, uint8_t, uint8_t, uint8_t, combination_t *(*)[TOTAL_CARDS]);
uint8_t fill_care_data(card_t *, care_data_t *, uint8_t, uint8_t);
void init_suits(uint8_t *, care_data_t *);
uint8_t find_best_suit_count(card_t *, uint8_t *, uint8_t);
void set_double_kickers(combination_t *, care_data_t *, uint8_t, uint8_t);
void print_item(uint8_t, uint8_t, uint8_t, combination_t *);
void set_combs(void);

void set_card(uint8_t);
combination_t *get_lowest(uint8_t, combination_t *(*)[TOTAL_CARDS]);

float get_pf_probs(card_t *, uint8_t, fi_t *);
float get_opponent_win_probability(card_t *, uint8_t, combination_t *, fi_t *);
/* END DEFINITIONS */

void *comb_func[COMB_ROYAL_FLUSH + 1];

static inline void print_card(uint8_t idx) {
	uint8_t suit, num;
	char c;

	suit = card_suit(idx);
	num = card_num(idx);

	if (num > 10) {
		switch (num) {
			case 11:
				c = 'J';
				break;
			case 12:
				c = 'Q';
				break;
			case 13:
				c = 'K';
				break;
			case 14:
				c = 'A';
				break;
			default:
				c = '?';

		}
		printf("%c%s", c , v_suits[suit]);
	}
	else
		printf("%u%s", num, v_suits[suit]);
}




void main_loop(void);
void init_system(void);
void init_ctx(void);
void create_wins(void);

void sigint_handler(int);
void print_bottom(void);
void print_main(void);
void process(void);
void process_preflop(void);
void process_flop(void);
void process_turn(void);
void process_river(void);
void process_finish(void);
int parse_buf(card_t *, uint8_t);
uint8_t get_suit(char);
void echo_input(card_t *, char *, int);
int valid_cards(card_t *, uint8_t);

float get_flop_prob(card_t *, combination_t *, int, fi_t *);
float get_turn_prob(card_t *, combination_t *, int, fi_t *);
float get_river_prob(card_t *, combination_t *, int, fi_t *);

uint8_t get_players(void);
void save_players(uint8_t);
void save_state(uint8_t);
void process_folders(void);
float calc_oprob(float, int);

enum {
	ST_WAIT_FOR_PREFLOP,
	ST_WAIT_FOR_FLOP,
	ST_WAIT_FOR_TURN,
	ST_WAIT_FOR_RIVER,
	ST_WAIT_FOR_FINISH
};


enum {
	ST_PARSE_WAIT_NUM,
	ST_PARSE_WAIT_ZERO,
	ST_PARSE_WAIT_SUIT,
	ST_PARSE_ERROR
};

typedef struct idx_s {
	uint8_t idx0,idx1;
} idx_t;

typedef struct cidx_s {
	int cnt;
	idx_t idxs[TOTAL_CARDS * TOTAL_CARDS];
} cidx_t;



typedef struct ctx_s {
	uint8_t bs, stop;
	uint8_t buf_idx;
	uint8_t players, folders;
	uint8_t state, error;
	float prob, mprob, oprob;
	uint8_t changed;
	card_t cards[7];	
	combination_t combination;
	cidx_t cidxs[MAX_COMB + 1];
	fi_t fi;
} ctx_t;

typedef struct tha_s {
	int players, folders;
	card_t *cards;
} tha_t;

void get_opp_combs(card_t *, combination_t *, int, cidx_t *);

#define MAX_PLAYERS 6
#define MAX_CARD_BUF 16

#define PL_FILE	"/tmp/pplayers"
#define ST_FILE "/tmp/pk_state"


enum {  
        ST_PREFLOP,
        ST_FLOP,
        ST_TURN,
        ST_RIVER,
        ST_FINISH
};



#define RET_STRAIGHT_DRAW	0x1
#define RET_GUT_SHOT		0x2
#define RET_FLUSH_DRAW		0x4
#define RET_OVER_PAIR		0x8
#define RET_TOP_PAIR		0x10
#define RET_MIDDLE_PAIR		0x20
#define RET_BOTTOM_PAIR		0x40

void *tf(void *);
void helper_loop(void);
void set_combination(card_t *, combination_t *, uint8_t);
int _check_straight_draw(card_t *, uint8_t);
int _check_flush_draw(card_t *, uint8_t);
int _pair_state(card_t *, uint8_t, uint8_t);

#endif
