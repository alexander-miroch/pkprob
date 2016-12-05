#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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

char *combs[10] = {
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

unsigned char *v_suits[4] = {
	"\xe2\x99\xa0",
	"\xe2\x99\xa3",
	"\xe2\x99\xa2",
	"\xe2\x99\xa1"
};


#define	TR_COMB_POSSIBLE	0x0
#define	TR_COMB_IMPOSSIBLE	0x1


typedef struct card_s {
	uint8_t num;
	uint8_t suit;
} card_t;

#define TOTAL_CARDS	52
#define SUIT_CARDS	13

#define MATRIX_WHOLE	255


#define MYCARDS_CONTEXT		0x0
#define OPPCARDS_CONTEXT	0x1

static void init_tr(card_t *);
static void init_opp(card_t *);
static void display_tr(void);
static void display_opp(void);

typedef struct combination_s {
	uint8_t type;
	uint8_t high_card, low_card;
	uint8_t kicker[2];
} combination_t;

inline uint8_t card_idx(card_t *card) {
	return card->suit * SUIT_CARDS + (card->num - 2);
}

inline uint8_t card_num(uint8_t idx) {
	return idx % SUIT_CARDS + 2;
}

inline uint8_t card_suit(uint8_t idx) {
	return (uint8_t) idx / SUIT_CARDS;
}

inline void simple_sort(uint8_t *array, int count) {
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

inline uint8_t max(uint8_t *val, int count) {
	int i, max = 0;

	for (i = 0; i < count; i++) {
		if (val[i] > max)
			max = val[i];
	}
	return max;
}

inline uint8_t min(uint8_t *val, int count) {
	int i, min = CARD_ACE + 1;

	for (i = 0; i < count; i++) {
		if (val[i] < min)
			min = val[i];
	}
	return min;
}

inline combination_t *get_combination(uint8_t type, uint8_t high_card) {
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

	return combination;
}


inline void matrix_value(void *m) {
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

uint16_t set_matrix(uint8_t, uint8_t, combination_t *);
uint16_t set_matrix_line(uint8_t, combination_t *);
uint16_t set_matrix_square(combination_t *);
uint16_t set_matrix_single(uint8_t, uint8_t, combination_t *);
uint16_t save_straight(uint8_t, uint8_t, uint8_t, uint8_t *, uint8_t);

/* combination count */
uint16_t royal_flush_cc(card_t *, uint8_t);
uint16_t straight_flush_cc(card_t *, uint8_t);
uint16_t care_cc(card_t *, uint8_t);
uint16_t full_house_cc(card_t *, uint8_t);
uint16_t flush_cc(card_t *, uint8_t);
uint16_t straight_cc(card_t *, uint8_t);
uint16_t triplet_cc(card_t *, uint8_t);
uint16_t double_pair_cc(card_t *, uint8_t);
uint16_t single_pair_cc(card_t *, uint8_t);
uint16_t none_cc(card_t *, uint8_t);

uint16_t straight_common(uint8_t *, uint8_t, uint8_t, uint8_t);
uint8_t fill_care_data(card_t *, care_data_t *, uint8_t);
void init_suits(uint8_t *, care_data_t *);
uint8_t find_best_suit_count(card_t *, uint8_t *);
void set_double_kickers(combination_t *, care_data_t *, uint8_t);
void print_item(uint8_t, uint8_t, uint8_t, combination_t *);
void set_combs(void);

float get_opponent_win_probability(card_t *, uint8_t, combination_t *);
/* END DEFINITIONS */

void *comb_func[COMB_ROYAL_FLUSH + 1];

inline void print_card(uint8_t idx) {
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


uint8_t high_card;
uint32_t total_possible;
/* turn & river matrix */
combination_t *my_matrix[TOTAL_CARDS][TOTAL_CARDS];
combination_t *opp_matrix[TOTAL_CARDS][TOTAL_CARDS];
combination_t *(*tr_matrix)[TOTAL_CARDS];

void set_combs(void) {
	comb_func[COMB_NONE] = (void *) none_cc;
	comb_func[COMB_SINGLE_PAIR] = (void *) single_pair_cc;
	comb_func[COMB_DOUBLE_PAIR] = (void *) double_pair_cc;
	comb_func[COMB_TRIPLET] = (void *) triplet_cc;
	comb_func[COMB_STRAIGHT] = (void *) straight_cc;
	comb_func[COMB_FLUSH] = (void *) flush_cc;
	comb_func[COMB_FULL_HOUSE] = (void *) full_house_cc;
	comb_func[COMB_CARE] = (void *) care_cc;
	comb_func[COMB_STRAIGHT_FLUSH] = (void *) straight_flush_cc;
	comb_func[COMB_ROYAL_FLUSH] = (void *) royal_flush_cc;
}

int main(int argc, char *argv[]) {
	card_t cards[7];
	float o_prob;
	int i,j;
	uint16_t cc;
	combination_t *combination;
	unsigned char ch[3];

	/*cards[MY_CARD1].num = 3;
	cards[MY_CARD1].suit = SUIT_PIKI;
	cards[MY_CARD2].num = 8;
	cards[MY_CARD2].suit = SUIT_CHERVI;*/
	cards[MY_CARD1].num = 2;
	cards[MY_CARD1].suit = SUIT_BUBI;
	cards[MY_CARD2].num = 8;
	cards[MY_CARD2].suit = SUIT_CHERVI;

	cards[FLOP_CARD1].num = 10;
	cards[FLOP_CARD1].suit = SUIT_PIKI;
	cards[FLOP_CARD2].num = 3;
	cards[FLOP_CARD2].suit = SUIT_TREFI;
	cards[FLOP_CARD3].num = CARD_KING;
	cards[FLOP_CARD3].suit = SUIT_PIKI;

	cards[TURN_CARD].num = cards[RIVER_CARD].num = 0;
	set_combs();

	tr_matrix = my_matrix;
	//high_card = (cards[MY_CARD1].num > cards[MY_CARD2].num) ? cards[MY_CARD1].num : cards[MY_CARD2].num;
	init_tr(cards);

	/*		ch[0] = 0xE2;
			ch[1] = 0x99;
			ch[2] = 0xA0;
			write(1, ch, 3);*/


//		printf("%02X %02X ->%04X\n",ch[0],ch[1],i);

//		if (i < 128)
//		write(1, ch, 1);
//		else
//		write(1, ch, 2);

	//return;

/*	for (i = 0; i < 52; i++) {
		print_card(i);
		printf(" ");

	}*/


	
	for (i = 0; i < 5; i++) {
		print_card(card_idx(&cards[i]));

		if (i == 1) printf(" | ");
		printf(" ");
	}
	printf("\n");

	printf("Welcome!\n");

	for (i = COMB_ROYAL_FLUSH; i >= 0; i--) {
	//	printf("%x,%x\n",&comb_func[i], royal_flush_cc);
		cc = ((uint16_t (*)(card_t *, uint8_t)) (comb_func[i]))(cards, MYCARDS_CONTEXT);
		printf("%s cc=%u\n",combs[i],cc);
	}

/*
	cc += royal_flush_cc(cards, MYCARDS_CONTEXT);
	printf("royal cc=%d\n",cc);

	cc += straight_flush_cc(cards, MYCARDS_CONTEXT);
	printf("straight flush cc=%d\n",cc);

	cc += care_cc(cards, MYCARDS_CONTEXT);
	printf("care cc=%d\n",cc);

	cc += full_house_cc(cards, MYCARDS_CONTEXT);
	printf("full house cc=%d\n",cc);

	cc += flush_cc(cards, MYCARDS_CONTEXT);
	printf("flush cc=%d\n",cc);

	cc += straight_cc(cards, MYCARDS_CONTEXT);
	printf("straight cc=%d\n",cc);

	cc += triplet_cc(cards, MYCARDS_CONTEXT);
	printf("triplet cc=%d\n",cc);

	cc += double_pair_cc(cards, MYCARDS_CONTEXT);
	printf("double pair cc=%d\n",cc);

	cc += single_pair_cc(cards, MYCARDS_CONTEXT);
	printf("single pair cc=%d\n",cc);

	cc += none_cc(cards, MYCARDS_CONTEXT);
	printf("rest cc=%d\n",cc);
*/	


	//display_tr();

	o_prob = 0;
	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < i; j++) {

			if (tr_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				printf("Something goes wrong...\n");
				return;
			}
			
			
			combination = (combination_t *) tr_matrix[i][j];

			//print_item(i, j, 0, combination);
		
			cards[TURN_CARD].num = card_num(i);
			cards[TURN_CARD].suit = card_suit(i);
			cards[RIVER_CARD].num = card_num(j);
			cards[RIVER_CARD].suit = card_suit(j);
		
	//		printf("%d-%d",card_num(i),card_num(j));
	//		printf("%u-%u turn=%d,m=%d rov=%d,m=%d\n",i,j, cards[TURN_CARD].num, cards[TURN_CARD].suit, cards[RIVER_CARD].num, cards[RIVER_CARD].suit);
			
			//o_prob += o_royal_flush_prob(cards, 4);
	
			o_prob += get_opponent_win_probability(cards, 4, combination);

			
			//o_prob += o_straight_flush(cards,4)
		}
		
	}
	
	o_prob /= 2162;
	

	printf("rf=%f\n",o_prob);
}

uint16_t rowcol_sum2(uint8_t idx0, uint8_t idx1, uint8_t idx2, uint8_t idx3, combination_t *(*comb)[TOTAL_CARDS]) {
	uint8_t i,j;
	uint16_t cc;

	for (i = 0; i < TOTAL_CARDS; i++) {
		if (tr_matrix[i][idx1] == (void *) TR_COMB_IMPOSSIBLE)
			continue;
	
		if (tr_matrix[i][idx1] == (void *) TR_COMB_POSSIBLE) 
			continue;
	//	printf("skipping "); print_card(i); printf(" "); print_card(idx1); printf("\n");
		cc++;
	}

	for (i = 0; i < TOTAL_CARDS; i++) {
		if (tr_matrix[i][idx0] == (void *) TR_COMB_IMPOSSIBLE)
			continue;
	
		if (tr_matrix[i][idx0] == (void *) TR_COMB_POSSIBLE) 
			continue;
	//	printf("skipping "); print_card(i); printf(" "); print_card(idx0); printf("\n");
		cc++;
	}

	for (j = 0; j < TOTAL_CARDS; j++) {
		if (tr_matrix[idx0][j] == (void *) TR_COMB_IMPOSSIBLE)
			continue;
	
		if (tr_matrix[idx0][j] == (void *) TR_COMB_POSSIBLE) 
			continue;

		if (j == idx0 || j == idx1)
			continue;
	//	printf("skipping "); print_card(idx0); printf(" "); print_card(j); printf("\n");
		cc++;
	}

	for (j = 0; j < TOTAL_CARDS; j++) {
		if (tr_matrix[idx1][j] == (void *) TR_COMB_IMPOSSIBLE)
			continue;
	
		if (tr_matrix[idx1][j] == (void *) TR_COMB_POSSIBLE) 
			continue;

		if (j == idx0 || j == idx1)
			continue;

	//	printf("skipping "); print_card(idx1); printf(" "); print_card(j); printf("\n");
		cc++;
	}

}


inline uint16_t xxx(uint8_t *cnts, uint8_t idx0, uint8_t idx1) {
	int i,j;
	uint8_t k = 0;

	return 50;
	for (i = 0; i < TOTAL_CARDS; i++) {
		if (i == idx0 || i == idx1)
			continue;

		cnts[k] = 0;
		for (j = 0; j < i; j++) {
			if (j == idx0 || j == idx1)
				continue;
			if (tr_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	/*
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				continue;
			}*/

	//		printf("idx=%u card: ",k); print_card(i); print_card(j); printf("\n");
			cnts[k]++;
		}
		if (cnts[k]) 
			k++;
	}

	return k;
}

uint16_t rowcol_sum(uint8_t idx0, uint8_t idx1, combination_t *(*comb)[TOTAL_CARDS]) {
	register int i, j;
	register uint16_t cc = 0;

	//printf("rowcol for: ");
//	print_card(idx0); printf(" "); print_card(idx1); printf("\n");



	for (i = 0; i < TOTAL_CARDS; i++) {
		if (tr_matrix[i][idx1] == (void *) TR_COMB_IMPOSSIBLE)
			continue;
	
		if (tr_matrix[i][idx1] == (void *) TR_COMB_POSSIBLE) 
			continue;
	//	printf("skipping "); print_card(i); printf(" "); print_card(idx1); printf("\n");
		cc++;
	}

	for (i = 0; i < TOTAL_CARDS; i++) {
		if (tr_matrix[i][idx0] == (void *) TR_COMB_IMPOSSIBLE)
			continue;
	
		if (tr_matrix[i][idx0] == (void *) TR_COMB_POSSIBLE) 
			continue;
	//	printf("skipping "); print_card(i); printf(" "); print_card(idx0); printf("\n");
		cc++;
	}

	for (j = 0; j < TOTAL_CARDS; j++) {
		if (tr_matrix[idx0][j] == (void *) TR_COMB_IMPOSSIBLE)
			continue;
	
		if (tr_matrix[idx0][j] == (void *) TR_COMB_POSSIBLE) 
			continue;

		if (j == idx0 || j == idx1)
			continue;
	//	printf("skipping "); print_card(idx0); printf(" "); print_card(j); printf("\n");
		cc++;
	}

	for (j = 0; j < TOTAL_CARDS; j++) {
		if (tr_matrix[idx1][j] == (void *) TR_COMB_IMPOSSIBLE)
			continue;
	
		if (tr_matrix[idx1][j] == (void *) TR_COMB_POSSIBLE) 
			continue;

		if (j == idx0 || j == idx1)
			continue;

	//	printf("skipping "); print_card(idx1); printf(" "); print_card(j); printf("\n");
		cc++;
	}

	//printf("cc=%d\n",cc);
	//display_tr();
	//exit(0);
	return cc;
	//printf("without: "); print_card(idx0); printf(" "); print_card(idx1); printf(" skip %u\n",cc);


	for (i = 0; i < TOTAL_CARDS; i++) {
	//	if (i == idx0 || i == idx1)
	//		continue;

		for (j = 0; j < i; j++) {
	//		if (j == idx0 || j == idx1) {
	//			continue;
	//		}
			if (tr_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				continue;
			}
			if (j == idx0 || j == idx1 || i == idx0 || i == idx1) {
		//		printf("skip for: ");print_card(i); printf(" "); print_card(j); printf("\n");
				continue;
			}
			
			

			//printf("valid for: ");print_card(i); printf(" "); print_card(j); printf("\n");
			cc++;
		}
	}

//	printf("skipped %u\n",cc * 2);
//	printf("cc=%u\n",cc);
//	exit(0);
	/*for (j = 0; j < idx1; j++) {
		if (tr_matrix[idx0][j] == (void *) TR_COMB_IMPOSSIBLE)
			continue;

		if (tr_matrix[idx0][j] == (void *) TR_COMB_POSSIBLE) {
			continue;
		}

		printf("excluding: ");print_card(idx0); printf(" "); print_card(j); printf("\n");
		cc++;
	}

	printf("precc=%u\n",cc);
	for (i = 0; i <= idx0; i++) {
		if (tr_matrix[i][idx1] == (void *) TR_COMB_IMPOSSIBLE)
			continue;

		if (tr_matrix[i][idx1] == (void *) TR_COMB_POSSIBLE) {
			continue;
		}

		printf("excluding col: ");print_card(i); printf(" "); print_card(idx1); printf("\n");
		cc++;
	}

	printf("precc2=%u\n",cc);
*/
	return cc * 2;
}

float get_opponent_win_probability(card_t *cards, uint8_t players, combination_t *combination) {
	float prob;
	uint16_t cc = 0;
	uint32_t vcc = 0, diff;
	int i,j,l;
	combination_t *opp_comb;
	float v[5], sumprob;
	uint32_t cnt=0;
	combination_t *(*comb)[52];


	if (players > 4) {
		printf("too much players");
		exit(0);
	}

	total_possible = 0;
	tr_matrix = opp_matrix;
	//init_tr
	init_tr(cards);
//	display_opp();
	//display_tr();

//	printf("Will check opp prob with these cards:\n");
/*	for (i = 0; i < 7; i++) {
		print_card(card_idx(&cards[i]));

		if (i == 1) printf(" | ");
		printf(" ");
		
	}
	printf("\n");*/
	cards += 2;	

//	printf("co=%s\n",combs[combination->type]);
//	prob = royal_flush_prob(cards, OPPCARDS_CONTEXT);

	for (i = COMB_ROYAL_FLUSH; i >= combination->type; i--) {
	//	if (i == combination->type) {
	//		break;
	//	} 

	/*	printf("trying comb %s -> %x-%x\n", combs[i], comb_func[i], care_cc);
		if (comb_func[i] == care_cc) {
			for (j=0; j<5;j++) {
				print_card(card_idx(&cards[j]));
				printf( "-");
			}
			printf("\n");
		}*/

		cc += ((uint16_t (*)(card_t *, uint8_t)) (comb_func[i]))(cards, MYCARDS_CONTEXT);
	//	printf("%s cc=%u\n",combs[i],cc);
	}

	//printf("now cc=%u\n", cc);
	/* fix equal combs */
	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < i; j++) {
			if (tr_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				continue;
			}

			opp_comb = (combination_t *) tr_matrix[i][j];
			if (opp_comb->type == combination->type) {
//				printf("will fix %s, cards: ", combs[opp_comb->type], combination);
//				print_card(i);
//				printf(" ");
//				print_card(j);
//				printf("\n");
				
				if (opp_comb->high_card > combination->high_card) {
				//	printf("opp win due to high card %u vs my %u\n",opp_comb->high_card, combination->high_card);
					continue;
				} else if (opp_comb->high_card < combination->high_card) {
				//	printf("I win due to high card %u vs my %u\n",opp_comb->high_card, combination->high_card);
					tr_matrix[i][j] = tr_matrix[j][i] = (void *) TR_COMB_POSSIBLE;
					cc -= 2;
					continue;
				}

				if (opp_comb->low_card) {
					if (opp_comb->low_card > combination->low_card) {
				//		printf("opp win due to low card %u vs my %u\n",opp_comb->low_card, combination->low_card);
						continue;
					} else if (opp_comb->low_card < combination->low_card) {
				//		printf("I win due to low card %u vs my %u\n",opp_comb->low_card, combination->low_card);
						tr_matrix[i][j] = tr_matrix[j][i] = (void *) TR_COMB_POSSIBLE;
						cc -= 2;
						continue;
					}

				}	

				/* Kickers */
				for (l = 0; l < 3; l++) {
					if (!opp_comb->kicker[l])
						continue;

					if (opp_comb->kicker[l] == combination->kicker[l]) {
				//		printf("kicker %d equals %u\n", l, opp_comb->kicker[l]);

						/* Draw */
						if (l == 2) {
			//				printf("DRAW!\n");
							// treat draw as my lose
							
						}
						continue;
					}

					if (opp_comb->kicker[l] < combination->kicker[l]) {
				//		printf("I win due to kicker %u my=%u his=%u\n", l,combination->kicker[l], opp_comb->kicker[l]);
						tr_matrix[i][j] = tr_matrix[j][i] = (void *) TR_COMB_POSSIBLE;
						cc -= 2;
						break;
					} else {
				//		printf("I lose due to kicker %u my=%u his=%u\n", l,combination->kicker[l], opp_comb->kicker[l]);
						break;
					}
				}
			}
		}
	}
	

	//printf("and now cc=%u\n", cc);

	if (!cc) {
		//printf("zero prob\n");
		tr_matrix = my_matrix;
		return 0;
	}

	struct cnts_s {
		uint8_t num;
		uint8_t primary;
		uint8_t neigh[TOTAL_CARDS];
	};

	

	//struct cnts_s cnts[TOTAL_CARDS];

	//memset(0, cnts, sizeof(struct cnts_s) * TOTAL_CARDS);
	uint8_t cnts[TOTAL_CARDS];
	uint8_t k = 0;
	for (i = 0; i < TOTAL_CARDS; i++) {
		cnts[k] = 0;
		for (j = 0; j < TOTAL_CARDS; j++) {
			if (tr_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				continue;
			}

	//		printf("idx=%u card: ",k); print_card(i); print_card(j); printf("\n");
			cnts[k]++;
		}
		if (cnts[k]) 
			k++;
	}

	uint64_t sum = 0;
	//printf("--- %u\n", k);
	for (i = 0; i < k; i++) {
		sum += cnts[i] * cnts[i];
	//	printf("s=%llu\n",sum);
	}

	//printf("cc=%u %u %u tp=%u\n",cc,sum, 4*sum, total_possible);
	v[1] = (float) ((float) cc + (float) 2.0 - ((float) 4.0 * (float)sum)/(float)cc) / 1806;
printf("v1var1=%g\n",v[1]);

	char c = '.';
	//write(1, &c, 1);

	//setvbuf(stdout,NULL,_IONBF,0);
	/*for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < i; j++) {
			if (tr_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				continue;
			}
			
			
			k= xxx(cnts, i, j);
		//	printf("k=%d\n",k);
			for (l = 0; l  < k; l++) {
				sum += cnts[l] * cnts[l];
			//	printf("s=%llu\n",sum);
			}
			
		}
	}*/

	//printf("s=%llu\n",sum);

	tr_matrix = my_matrix;
	return 0;
//printf("v1var1=%g\n",v[1]);

//	v[1] = ((float) 1.0/ (float) cc) * ((float) vcc / 1806);
/*	exit(0);

		tr_matrix = my_matrix;
	return 0;*/

	for (i = 0; i < players; i++)
		v[i] = 0;

	v[0] = (float) cc / (float) total_possible;

	vcc = l = diff = 0;
	//comb = malloc(sizeof(combination_t) * TOTAL_CARDS * TOTAL_CARDS);
	comb = tr_matrix;
	//printf("cnt=%u\n",cnt);
	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < i; j++) {
			if (tr_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
	//			printf("i=%u, %u\n");
				l++;
				continue;
			}

//			memcpy (comb, tr_matrix, sizeof(combination_t) * TOTAL_CARDS * TOTAL_CARDS);
//			printf(" vcc=%u\n",vcc);
			//vcc += rowcol_sum(i,j, comb);
			diff = rowcol_sum(i,j, comb);
			vcc += (cc - diff);
//			printf("now vcc is %u\n",vcc);
		//	xxx = rowcol_sum(i,j, comb);
		//	vcc += (cc - xxx);
		//	printf(" avcc=%u\n",vcc);
//			free(comb);
			cnt++;
	//		printf("cnt=%u\n",cnt);
			
		}
	}

	v[1] = ((float) 2.0/ (float) cc) * ((float) vcc / 1806);
//printf("v1var2=%g\n",v[1]);


	//exit(0);
	sumprob = 2*v[0] - v[0]*v[1];

	//free(comb);
	//printf("%u/%u l=%u cnt=%u v=%g v1=%g sumprob=%g\n",cc,total_possible,l,cnt,v[0],v[1],sumprob);
	if (cnt * 2 != cc) {
		printf("Something goes wrong again...");
		exit(0);
	}

//	printf("VCC=%u\n",vcc);


	tr_matrix = my_matrix;
	return sumprob;

//	prob += flush_straight_prob(cards, OPPCARDS_CONTEXT);

//	if (combination->type > COMB_STRAIGHT_FLUSH)
//		return cc += straight_flush_cc(cards, c

}

uint16_t set_matrix_single(uint8_t idx0, uint8_t idx1, combination_t *combination) {
	uint16_t cc = 0;
	uint8_t kicker[4];
	combination_t *other;

	kicker[0] = card_num(idx0);
	kicker[1] = card_num(idx1);
	kicker[2] = combination->kicker[0];

	simple_sort(kicker, 3);
	other = get_combination(combination->type, combination->high_card);
	other->kicker[0] = kicker[2];

	//		printf("%d qqqqset %u-%u and %u-%u hc=%u kicker=%u,%u\n",cc, card_num(idx0),card_suit(idx0),card_num(idx1),card_suit(idx1),other->high_card, other->kicker[0], other->kicker[1]);
	cc = set_matrix(idx0, idx1, other);
	return cc;
	
}

uint16_t set_matrix_square(combination_t *combination) {
	int i, j;
	uint16_t cc = 0;
	uint8_t num0, num1 ;
	combination_t *other;
	uint8_t kicker[5];

	for (i = 0; i < TOTAL_CARDS; i++) {

		for (j = 0; j < i; j++) {
			kicker[0] = card_num(i);
			kicker[1] = card_num(j);
			kicker[2] = combination->kicker[0];
			kicker[3] = combination->kicker[1];
			kicker[4] = combination->kicker[2];
			simple_sort(kicker, 5);
			
			other = get_combination(combination->type, combination->high_card);
			other->low_card = combination->low_card;
			other->kicker[0] = kicker[4];

			
			other->kicker[1] = other->kicker[2] = 0;
			if (combination->kicker[1]) {
				other->kicker[1] = kicker[3];
				if (combination->kicker[2])
					other->kicker[2] = kicker[2];
			}

	/*		int x;
			printf("all kik: ");
			for (x=0; x<5; x++) {
				printf("%u ", kicker[x]);
			}
			printf("\n");*/
			

	
	//		printf("%d set %u-%u and %u-%u hc=%u kicker=%u,%u,%u\n",cc, card_num(i),card_suit(i),card_num(j),card_suit(j),other->high_card, other->kicker[0], other->kicker[1], other->kicker[2]);
			cc += set_matrix(i, j, other);
//			printf("now cc=%u idx=%u-%u\n",cc ,i, j);
		
		}
	}

	//free(combination);
	//printf("cc=%d\n",cc);
	return cc;
}

uint16_t set_matrix_line(uint8_t v1, combination_t *combination) {
	int i;
	uint8_t num;
	uint16_t cc = 0;
	combination_t *other;
	uint8_t kicker[4];

	for (i = 0; i < TOTAL_CARDS; i++) {
		num = card_num(i);

		kicker[0] = card_num(i);
		kicker[1] = combination->kicker[0];
		kicker[2] = combination->kicker[1];
		kicker[3] = combination->kicker[2];
		simple_sort(kicker, 4);

		other = get_combination(combination->type, combination->high_card);
		other->low_card = combination->low_card;
		other->kicker[0] = kicker[3];

		other->kicker[1] = other->kicker[2] = 0;
		if (combination->kicker[1]) {
			other->kicker[1] = kicker[2];
			if (combination->kicker[2])
				other->kicker[2] = kicker[1];
		}

	//		printf("1set %u-%u and %u-%u hc=%u kicker=%u %u %u\n", card_num(v1),card_suit(v1),card_num(i),card_suit(i),other->high_card, other->kicker[0], other->kicker[1], other->kicker[2]);
			cc += set_matrix(v1, i, other);
	}	


	return cc;
	
}

void print_item(uint8_t i, uint8_t j, uint8_t cc, combination_t *c) {
	char hc, lc, k0, k1, k2;
	char *vals[16] = {
		"0", "0", "2", "3", "4","5","6", "7","8","9","10","J","Q","K","A"
	};

	printf("cc=%u set ",cc);
	print_card(i);
	printf("-");
	print_card(j);
	

	printf(" i=%u,j=%u c=%s, hc=%s, lc=%s, k=%s,%s,%s\n", i, j, combs[c->type], vals[c->high_card], vals[c->low_card], vals[c->kicker[0]], vals[c->kicker[1]], vals[c->kicker[2]]);
}

uint16_t set_matrix(uint8_t v1, uint8_t v2, combination_t *combination) {
	uint32_t cc = 0;
	int i, j;

	if (v1 == MATRIX_WHOLE && v2 == MATRIX_WHOLE) {
		for (i = 0; i < TOTAL_CARDS; i++) {
			for (j = 0; j < i; j++) {
				if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
					tr_matrix[i][j] = combination;
					cc++;
		//			print_item(i,j,cc,combination);
				}
	
				if (tr_matrix[j][i] == (void *) TR_COMB_POSSIBLE) {
					tr_matrix[j][i] = combination;
					cc++;
				//	print_item(j,i,cc,combination);
				}
			}
		
		}

		return cc;
	}


	if (v2 == MATRIX_WHOLE) {
		for (i = 0; i < TOTAL_CARDS; i++) {
	//		printf("m=%x\n", tr_matrix[i][v1]);
			if (tr_matrix[i][v1] == (void *) TR_COMB_POSSIBLE) {
				tr_matrix[i][v1] = combination;
				cc++;
		//			print_item(i,v1,cc,combination);
			} else {
		//		printf(" %u %u: ",i,v1);
			//	matrix_value(tr_matrix[i][v1]);
		//		printf("\n");
			}

			if (tr_matrix[v1][i] == (void *) TR_COMB_POSSIBLE) {
				tr_matrix[v1][i] = combination;
				cc++;
		//			print_item(v1,i,cc,combination);
			} else {
		//		printf(" %u %u: ",v1,i);
			//	matrix_value(tr_matrix[v1][i]);
		//		printf("\n");
			}
		}		
		return cc;
	} 
	
	if (tr_matrix[v1][v2] == (void *) TR_COMB_POSSIBLE) {
		tr_matrix[v1][v2] = combination;
		cc++;
		//			print_item(v1,v2,cc,combination);
	}

	if (tr_matrix[v2][v1] == (void *) TR_COMB_POSSIBLE) {
		tr_matrix[v2][v1] = combination;
		cc++;
				//	print_item(v2,v1,cc,combination);
	}

	return cc;
}

uint16_t save_straight(uint8_t max, uint8_t holes, uint8_t suit, uint8_t *needed, uint8_t type) {
	combination_t *combination;
	uint8_t idx0, idx1;
	uint16_t cc = 0;
	card_t card0, card1;
	int i, j;


	combination = get_combination(type, max);
	if (!holes) {
		cc= set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination);
	//	printf("nh cc=%u\n",cc);
		return cc;
	}

	card0.suit = suit;
	card0.num = needed[0];
	idx0 = card_idx(&card0);
	if (holes == 1) {
		if (suit == SUIT_ANY) {
			for (i = 0; i <= SUIT_CHERVI; i++) {
				card0.suit = i;
				idx0 = card_idx(&card0);
		//		printf("set idxs %u(%u m=%u) cc=%u\n",idx0,card0.num,card0.suit,cc);
				cc += set_matrix(idx0, MATRIX_WHOLE, combination);
		//		printf("now cc=%u\n",cc);
			}		
	
		} else
			cc = set_matrix(idx0, MATRIX_WHOLE, combination);

	//	printf("nh2 idx=%u cc=%u\n",idx0, cc);
		return cc;
	}

	card1.suit = suit;
	card1.num = needed[1];
	idx1 = card_idx(&card1);	
	if (suit == SUIT_ANY) {
		for (i = 0; i <= SUIT_CHERVI; i++) {
			for (j = 0; j <= SUIT_CHERVI; j++) {
				card0.suit = i;
				card1.suit = j;
				idx0 = card_idx(&card0);
				idx1 = card_idx(&card1);
		//		printf("set idxs %u(%u m=%u) %u(%u m=%u) cc=%u\n",idx0,card0.num,card0.suit,idx1,card1.num,card1.suit,cc);
				cc += set_matrix(idx0, idx1, combination);
		//		printf("now cc=%u\n",cc);
			
				
			}
		}	
	} else 
		cc = set_matrix(idx0, idx1, combination);
//	printf("cc11=%d\n",cc);
	return cc;
}

void set_double_kickers(combination_t *combination, care_data_t *care_data, uint8_t idx) {
	switch (idx) {
		case 0:
			combination->kicker[0] = care_data[1].num;
			combination->kicker[1] = care_data[2].num;
			combination->kicker[2] = care_data[3].num;
			break;
		case 1:
			combination->kicker[0] = care_data[0].num;
			combination->kicker[1] = care_data[2].num;
			combination->kicker[2] = care_data[3].num;
			break;
		case 2:	
			combination->kicker[0] = care_data[0].num;
			combination->kicker[1] = care_data[1].num;
			combination->kicker[2] = care_data[3].num;
			break;
		case 3:
		case 4:
			combination->kicker[0] = care_data[0].num;
			combination->kicker[1] = care_data[1].num;
			combination->kicker[2] = care_data[2].num;
			break;
	}
}

uint16_t none_cc(card_t *cards, uint8_t context) {
	uint8_t max;
	combination_t *combination;
	uint16_t cc = 0;

	max = cards[MY_CARD1].num > cards[MY_CARD2].num ? cards[MY_CARD1].num : cards[MY_CARD2].num;
	combination = get_combination(COMB_NONE, max);
	cc = set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination);
	
	return cc;
}

uint16_t single_pair_cc(card_t *cards, uint8_t context) {
	care_data_t care_data[5];
	combination_t *combination;
	uint8_t care_data_count;
	uint16_t cc = 0;
	int i, j, k, l;
	uint8_t idx = 255;
	uint8_t num, suit, max;
	card_t card0, card1, temp_card;
	uint8_t suits[4], supp_suits[4];
	uint8_t idx0, idx1;
	

	care_data_count = fill_care_data(cards, care_data, 0);
	if (care_data_count < 4)
		return 0;

		
/*
	if (cards[MY_CARD1].num == cards[MY_CARD2].num) {
		combination = get_combination(COMB_SINGLE_PAIR, cards[MY_CARD1].num);
		cc = set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination);
		return cc;
	}

	for (j = 0; j < care_data_count; j++) {
		if (care_data[j].cnt == 2) {
			idx = j;
			break;
		}
	}

	combination = NULL;
	if (cards[FLOP_CARD1].num == cards[FLOP_CARD2].num) {
		combination = get_combination(COMB_SINGLE_PAIR, cards[FLOP_CARD1].num);
	} else if (cards[FLOP_CARD2].num == cards[FLOP_CARD3].num) {
		combination = get_combination(COMB_SINGLE_PAIR, cards[FLOP_CARD2].num);
	} else if (cards[FLOP_CARD1].num == cards[FLOP_CARD3].num) {
		combination = get_combination(COMB_SINGLE_PAIR, cards[FLOP_CARD3].num);
	} 

	if (combination) {
		set_double_kickers(combination, care_data, idx);
		cc = set_matrix_square(combination);
		return cc;
	}

	if (idx != 255) {
		for (j = FLOP_CARD1; j <= FLOP_CARD3; j++) {
			if (cards[j].num == cards[MY_CARD1].num) {
				combination = get_combination(COMB_SINGLE_PAIR, cards[j].num);
				combination->kicker[0] = cards[MY_CARD2].num;
			} else if (cards[j].num == cards[MY_CARD2].num) {
				combination = get_combination(COMB_SINGLE_PAIR, cards[j].num);
				combination->kicker[0] = cards[MY_CARD1].num;
			}
		}
	
		cc = set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination);
		return cc;
	}
*/

	//printf("cdc=%u\n",care_data_count);
	for (i = 0; i < care_data_count; i++) {
		if (care_data[i].cnt == 2) {
			combination = get_combination(COMB_SINGLE_PAIR, care_data[i].num);
			set_double_kickers(combination, care_data, i);
			cc = set_matrix_square(combination);
			return cc;		
		}
	
		combination = get_combination(COMB_SINGLE_PAIR, care_data[i].num);
		set_double_kickers(combination, care_data, i);
	
		card0.num = care_data[i].num;
		init_suits(suits, &care_data[i]);
		for (j = 0; j < 4; j++) {
			if (suits[j] != 255)
				continue;

			card0.suit = j;
			idx0 = card_idx(&card0);

		//	printf("will set %u-%u hc=%u, k=%u-%u-%u\n",card0.num,card0.suit,combination->high_card,combination->kicker[0],combination->kicker[1],combination->kicker[2]);
		//	printf("spcc=%u\n",cc);
			cc += set_matrix_line(idx0, combination);
		//	printf("aspcc=%u\n",cc);
		}
	
	}


	for (j = CARD_ACE; j >= 2; j--) {
		for (k = 0; k < 4; k++) {
			card0.num = card1.num = j;
			card0.suit = k;
		
			combination = get_combination(COMB_SINGLE_PAIR, card0.num);
			set_double_kickers(combination, care_data, 4);

			idx0 = card_idx(&card0);
			for (l = 0; l < k; l++) {
				if (l == k)
					continue;
					
				card1.suit = l;
				idx1 = card_idx(&card1);
			//	printf("%d any look for %u=%u and %u=%u hc=%u lc=%u k=%u-%u\n",cc, card0.num,card0.suit,card1.num,card1.suit,combination->high_card,combination->low_card,combination->kicker[0], combination->kicker[1]);
				cc += set_matrix(idx0, idx1, combination);
//				printf("cc=%d\n",cc);
			}
		}
	}


	return cc;
}

uint16_t double_pair_cc(card_t *cards, uint8_t context) {
	care_data_t care_data[5];
	uint8_t care_data_count;
	combination_t *combination;
	card_t card0, card1, temp_card;
	uint8_t suits[4], supp_suits[4];
	uint8_t num, cnt;
	int i, j, k, l;
	uint16_t cc = 0;
	uint8_t idx0, idx1, idx;
	uint8_t kicker;

	care_data_count = fill_care_data(cards, care_data, 0);
	for (i = 0; i < care_data_count; i++) {
		num = care_data[i].num;
		cnt = care_data[i].cnt;

		if (cnt >= 3)
			return 0;

		card0.num = num;
		if (cnt == 2) {
			for (j = i + 1; j < care_data_count; j++) {
				if (i == j)
					continue;
			
				card1.num = care_data[j].num;
				if (care_data[j].cnt >= 3)
					return 0;

				combination = get_combination(COMB_DOUBLE_PAIR, card0.num);
				combination->low_card = card1.num;
				if (i == 0) {
					combination->kicker[0] = (j == 1) ? care_data[2].num : care_data[1].num;
				} else
					combination->kicker[0] = care_data[0].num;
						
				init_suits(suits, &care_data[j]);
				if (care_data[j].cnt == 2) {
					cc += set_matrix_square(combination);
				} else if (care_data[j].cnt == 1) {
					for (k = 0; k < 4; k++) {
						if (suits[k] != 255)
							continue;
				
						card1.suit = k;
						idx1 = card_idx(&card1);						
						cc += set_matrix_line(idx1, combination);
					}
					/*init_suits(&supp_suits, &care_data[j]);
					for (k = 0; k < 4; k++) {
						if (suits[k] != 255)
							continue;
					
						card0.suit = k;
						for (l = 0; l < 4; l++) {
							if (supp_suits[l] != 255)
								continue;
						
							card1.suit = l;
							cc += set_matrix_line

						}
					}*/
					//cc += 
				}
			}

			kicker = (i == 0) ? care_data[1].num : care_data[0].num;
			/* fill any pairs */
			for (j = CARD_ACE; j >= 2; j--) {
				for (k = 0; k < 4; k++) {
					card0.num = card1.num = j;
					card0.suit = k;
	
					combination = get_combination(COMB_DOUBLE_PAIR, care_data[i].num);
					combination->low_card = card0.num;
					combination->kicker[0] = kicker;

					idx0 = card_idx(&card0);
					for (l = 0; l < k; l++) {
						if (l == k)
							continue;
					
						card1.suit = l;
						idx1 = card_idx(&card1);
	//				printf("%d any look for %u=%u and %u=%u hc=%u lc=%u k=%u-%u\n",cc, card0.num,card0.suit,card1.num,card1.suit,combination->high_card,combination->low_card,combination->kicker[0], combination->kicker[1]);
						cc += set_matrix(idx0, idx1, combination);
	//					printf("cc=%d\n",cc);
					}
				}
			}

			return cc;
		} else if (cnt == 1) {
			for (j = i + 1; j < care_data_count; j++) {
				if (i == j)
					continue;

				card1.num = care_data[j].num;
				if (care_data[j].cnt >= 3)
					return 0;

				combination = get_combination(COMB_DOUBLE_PAIR, card0.num);
				combination->low_card = card1.num;
				if (i == 0) {
					combination->kicker[0] = (j == 1) ? care_data[2].num : care_data[1].num;
				} else
					combination->kicker[0] = care_data[0].num;
						
				init_suits(suits, &care_data[i]);
				init_suits(supp_suits, &care_data[j]);
				if (care_data[j].cnt == 2) {
					for (k = 0; k < 4; k++) {
						if (suits[k] != 255)
							continue;
				
						card0.suit = k;
						idx0 = card_idx(&card0);						
						cc += set_matrix_line(idx0, combination);
					}
				} else if (care_data[j].cnt == 1) {
					for (k = 0; k < 4; k++) {
						if (suits[k] != 255)
							continue;

						card0.suit = k;
						idx0 = card_idx(&card0);
						for (l = 0; l < 4; l++) {
							if (supp_suits[l] != 255)
								continue;
						
							card1.suit = l;
							idx1 = card_idx(&card1);
		//			printf("%d 1+1 look for %u=%u and %u=%u hc=%u lc=%u k=%u-%u\n",cc, card0.num,card0.suit,card1.num,card1.suit,combination->high_card,combination->low_card,combination->kicker[0], combination->kicker[1]);
							cc += set_matrix(idx0, idx1, combination);	
						}
					}
				}

			}


		}

	}
	return cc;
}

uint16_t triplet_cc(card_t *cards, uint8_t context) {
	care_data_t care_data[5];
	uint8_t care_data_count;
	uint8_t num, cnt;
	int i, j;
	uint8_t suits[4];
	uint16_t cc = 0;
	combination_t *combination;
	card_t card0, card1, temp_card;
	uint8_t idx0, idx1, idx;
	uint8_t kicker0, kicker1;

//	init_suits(suits, &care_data[i]);

	care_data_count = fill_care_data(cards, care_data, 0);
	if (care_data_count < 3)
		return 0;

	for (i = 0; i < care_data_count; i++) {
		num = care_data[i].num;
		cnt = care_data[i].cnt;
		
		if (cnt > 3)
			return 0;

		if (i == 0) {
			kicker0 = care_data[1].num;
			kicker1 = care_data[2].num;
		} else {
			kicker0 = care_data[0].num;
			kicker1 = (i == 1) ? care_data[2].num : care_data[1].num;
		}
		combination = get_combination(COMB_TRIPLET, num);
		combination->kicker[0] = kicker0;
		combination->kicker[1] = kicker1;

		if (cnt == 3) {
			//cc = set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination);
			cc += set_matrix_square(combination);
			return cc;
		}
	
		card0.num = card1.num = num;
		init_suits(suits, &care_data[i]);
		if (cnt == 2) {
			for (j = 0; j < 4; j++) {
				if (suits[j] != 255)
					continue;

				card0.suit = j;
				idx0 = card_idx(&card0);
	//			printf("sqetting %u=%u hc=%u k=%u-%u\n",card0.num,card0.suit,combination->high_card);
	//			cc += set_matrix(idx0, MATRIX_WHOLE, combination);
				cc += set_matrix_line(idx0, combination);
	//			printf("cc=%u\n",cc);
			}
		} else if (cnt == 1) {
			idx = 0;
			for (j = 0; j < 4; j++) {
				if (suits[j] != 255)
					continue;

					if (!idx) {
						card0.suit = j;
					} else if (idx == 1) {
						card1.suit = j;
				//		printf("%d 1look for %u=%u and %u-%u hv=%d k=%u-%u\n",cc, card0.num,card0.suit,card1.num,card1.suit, combination->high_card, combination->kicker[0], combination->kicker[1]);
						idx0 = card_idx(&card0);
						idx1 = card_idx(&card1);
				//		printf("lf idxs %u-%u\n",idx0,idx1);
						cc += set_matrix(idx0, idx1, combination);
				//		printf("CCCC=%d\n",cc);
					} else if (idx == 2) {
						temp_card = card1;
						card1.suit = j;

				//		printf("%d look for %u=%u and %u-%u k=%u-%u\n", cc, card0.num,card0.suit,card1.num,card1.suit,combination->kicker[0], combination->kicker[1]);
						idx0 = card_idx(&card0);
						idx1 = card_idx(&card1);
				//		printf("lf idxs %u-%u\n",idx0,idx1);
						cc += set_matrix(idx0, idx1, combination);
				//		printf("CCCC=%d\n",cc);
				//		printf("%d look for %u=%u and %u-%u hc=%u k=%u-%u\n",cc, temp_card.num,temp_card.suit,card1.num,card1.suit, combination->high_card, combination->kicker[0], combination->kicker[1]);
						idx0 = card_idx(&temp_card);
				//		printf("lf idxs %u-%u\n",idx0,idx1);
						cc += set_matrix(idx0, idx1, combination);
				//		printf("CCCC=%d\n",cc);
					}
					idx++;
			}
		}
//		printf("n=%u\n",care_data[i].num);
	}

	return cc;
}

combination_t *get_flush_combination(uint8_t *sorted, uint8_t count) {
	int i, j;
	combination_t *combination;

/*	printf("flc=%u : ");
	for (i = 0; i < count; i++) {
		printf("%u ", sorted[i]);
	}
	printf("\n");*/

	combination = get_combination(COMB_FLUSH, sorted[count - 1]);
	combination->low_card = sorted[count - 2];
	for (j = 0, i = count - 3; i >= 0; j++, i--) {
		combination->kicker[j] = sorted[i];
	}
	
	return combination;
}

uint16_t flush_cc(card_t *cards, uint8_t context) {
	uint8_t best_count, best_suit;
	uint8_t num, high_card;
	combination_t *combination;
	int i, j, k, fin;
	card_t card0,card1;
	uint8_t idx0,idx1;
	uint16_t cc = 0;
	uint8_t comb = 0;
	uint8_t sorted[7], s_sorted[7], scount;
	uint8_t suit0, suit1, num0,num1;

	best_suit = find_best_suit_count(cards, &best_count);

	if (best_count < 3)
		return 0;

	j = 0;
	for (i = 0; i < 5; i++) {
		if (cards[i].suit == best_suit) {
			sorted[j++] = cards[i].num;
		}
	}
	int prevcc=0;
	simple_sort(sorted, best_count);
	for (i = 0; i < TOTAL_CARDS; i++) {
		suit0 = card_suit(i);
		num0 = card_num(i);
		for (j = 0; j < i; j++) {
			scount = best_count;
			suit1 = card_suit(j);
			num1 = card_num(j);
				
			if (best_count == 4) {
				if (suit0 != best_suit && suit1 != best_suit)
					continue;
			} else if (best_count == 3) {
				if (suit0 != best_suit || suit1 != best_suit)
					continue;
			}


			if (suit0 == best_suit) {
				memcpy(s_sorted, sorted, best_count);
				s_sorted[scount++] = num0;
				if (suit1 == best_suit)
					s_sorted[scount++] = num1;
				simple_sort(s_sorted, scount);
				combination = get_flush_combination(s_sorted, scount);
			} else if (suit1 == best_suit) {
				memcpy(s_sorted, sorted, best_count);
				s_sorted[scount++] = num1;
				simple_sort(s_sorted, scount);
				combination = get_flush_combination(s_sorted, scount);
			} else {
				combination = get_flush_combination(sorted, best_count);
			}

			prevcc = cc;
			cc += set_matrix(i, j, combination);
		/*	if (cc != prevcc) {
				printf("flush cards: ");
				print_card(i); printf(" "); print_card(j);
				printf(" hc=%u lc=%u k=%u,%u,%u\n",combination->high_card, combination->low_card, combination->kicker[0],combination->kicker[1], combination->kicker[2]);
				
			}*/
		}

	}

	return cc;
	//printf("c=%d %d\n",best_count,num);

}

uint16_t full_house_cc(card_t *cards, uint8_t context) {
	int i, j, k, l;
	uint8_t care_data_count;
	care_data_t care_data[5];
	combination_t *combination;
	card_t card0, card1, temp_card;
	uint8_t idx0, idx1;
	uint8_t cnt, suits[4], suits_supp[4], idx;
	uint16_t cc = 0;

	care_data_count = fill_care_data(cards, care_data, 0);

	/*care_data_count = 0;
	memset(care_data, 0, sizeof(care_data_t) * 2);
	for (j = 0, i = SUIT_CARDS - 1; i >= 0; i--) {
		if (counter[i].num != 255) {
			memcpy(&care_data[j], &counter[i], sizeof(care_data_t));
			j++;	
		}
	}*/

	if (care_data_count == 5)
		return 0;

	for (i = 0; i < care_data_count; i++) {
		cnt = care_data[i].cnt;

		if (care_data[i].num == 255)
			continue;

		/* care? */
		if (cnt == 4) 
			return 0;

		combination = get_combination(COMB_FULL_HOUSE, care_data[i].num);
		if (cnt == 1) {
			for (j = 0; j < care_data_count; j++) {
				if (i == j) 
					continue;

				if (care_data[j].cnt == 1) 
					continue;

				combination = get_combination(COMB_FULL_HOUSE, care_data[i].num);
				combination->low_card = care_data[j].num;

				card0.num = card1.num = care_data[i].num;
				card0.suit = card1.suit = 255;
				init_suits(suits, &care_data[i]);
				idx = 0;
				for (k = 0; k < 4; k++) {
					if (suits[k] != 255) {
						continue;
					}

					if (!idx) {
						card0.suit = k;
					} else if (idx == 1) {
						card1.suit = k;
	//					printf("%d 1look for %u=%u and %u-%u hv=%d k=%u\n",cc, card0.num,card0.suit,card1.num,card1.suit, combination->high_card, combination->kicker[0]);
						idx0 = card_idx(&card0);
						idx1 = card_idx(&card1);
				//		printf("lf idxs %u-%u\n",idx0,idx1);
						cc += set_matrix(idx0, idx1, combination);
				//		printf("CCCC=%d\n",cc);
					} else if (idx == 2) {
						temp_card = card1;
						card1.suit = k;

	//					printf("%d look for %u=%u and %u-%u hc=%u kicker=%u\n", cc, card0.num,card0.suit,card1.num,card1.suit, combination->high_card, combination->kicker[0]);
						idx0 = card_idx(&card0);
						idx1 = card_idx(&card1);
				//		printf("lf idxs %u-%u\n",idx0,idx1);
						cc += set_matrix(idx0, idx1, combination);
				//		printf("CCCC=%d\n",cc);
	//					printf("%d look for %u=%u and %u-%u hc=%u k=%u\n",cc, temp_card.num,temp_card.suit,card1.num,card1.suit, combination->high_card,  combination->kicker[0]);
						idx0 = card_idx(&temp_card);
				//		printf("lf idxs %u-%u\n",idx0,idx1);
						cc += set_matrix(idx0, idx1, combination);
				//		printf("CCCC=%d\n",cc);
					}
					idx++;
				}

			}
		} else if (cnt == 2) {
			for (j = 0; j < care_data_count; j++) {
				if (i == j) 
					continue;

				card0.num = care_data[i].num;
				card1.num = care_data[j].num;
				init_suits(suits, &care_data[i]);

				combination = get_combination(COMB_FULL_HOUSE, care_data[i].num);
				combination->low_card = care_data[j].num;

				for (k = 0; k < 4; k++) {
					if (suits[k] != 255)
						continue;

					card0.suit = k;
					idx0 = card_idx(&card0);
					if (care_data[j].cnt >= 2) {
	//					printf("%d qlook for %u=%u hc=%d k=%u-%u\n",cc, card0.num,card0.suit, combination->high_card, combination->kicker[0], combination->kicker[1]);
						cc += set_matrix(idx0, MATRIX_WHOLE, combination);
						//cc += set_matrix_line(idx0, combination);
					//	printf("CC=%d\n",cc);
						continue;
					}

					init_suits(suits_supp, &care_data[j]);
					for (l = 0; l < 4; l++) {
						if (suits_supp[l] != 255)
							continue;
						
						card1.suit = l;
						idx1 = card_idx(&card1);
		//				printf("lzook for %u=%u and %u-%u hc=%d k=%u-%u\n", card0.num,card0.suit,card1.num,card1.suit, combination->high_card,combination->kicker[0], combination->kicker[1]);
					//	printf("lf idxs %u-%u\n",idx0,idx1);
						cc += set_matrix(idx0,idx1, combination);
					//	printf("CC=%d\n",cc);
					}
				}
			}

/*			for (j = 0; j < care_data_count; j++) {
				if (i == j)
					continue;

				
		
			}		*/
		} else if (cnt == 3) {
			for (j = 0; j < care_data_count; j++) {
				if (i == j)
					continue;

				combination = get_combination(COMB_FULL_HOUSE, care_data[i].num);
				combination->low_card = care_data[j].num;

				if (care_data[j].cnt == 2) {
					cc += set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination);
					return cc;
				}

				card0.num  = care_data[j].num;
				init_suits(suits, &care_data[j]);

				for (k = 0; k < 4; k++) {
					if (suits[k] != 255)
						continue;

					card0.suit = k;
					idx0 = card_idx(&card0);
//					printf("%d ddlook for %u=%u k=%u-%u \n",cc, card0.num,card0.suit, combination->kicker[0], combination->kicker[1]);
					cc += set_matrix(idx0, MATRIX_WHOLE, combination);
					//printf("cc=%d\n",cc);
				}
			}

			/* fill any pairs */
			for (j = CARD_ACE; j >= 2; j--) {
				for (k = 0; k < 4; k++) {
					card0.num = card1.num = j;
					card0.suit = k;
	
					combination = get_combination(COMB_FULL_HOUSE, care_data[i].num);
					combination->low_card = card0.num;

					idx0 = card_idx(&card0);
					for (l = 0; l < k; l++) {
						if (l == k)
							continue;
					
						card1.suit = l;
						idx1 = card_idx(&card1);
					//	printf("%d any look for %u=%u and %u=%u k=%u-%u\n",cc, card0.num,card0.suit,card1.num,card1.suit,combination->kicker[0], combination->kicker[1]);
						cc += set_matrix(idx0, idx1, combination);
	//					printf("cc=%d\n",cc);
					}
				}
			}
		}
	}	


	//printf("sz=%d\n",care_data_count);
/*
	for (i = 0; i < care_data_count; i++) {
		printf("xxxFH%d \n",i);
		for (j = 0; j < 3; j++) {
			if (care_data[i].suit[j] == 255)
				continue;
	
			printf("C1=%u-%u cnt=%u\n",care_data[i].num,care_data[i].suit[j],care_data[i].cnt);
		}

	}
*/
	return cc;

}

uint8_t fill_care_data(card_t *cards, care_data_t *care_data, uint8_t min) {
	int i, j;
	uint8_t idx;
	care_data_t counter[SUIT_CARDS];

	memset(counter, 255, sizeof(care_data_t) * SUIT_CARDS);
	for (i = 0, j = 0; i < 5; i++) {
		idx = cards[i].num - 2;
		if (counter[idx].num == 255) {
			counter[idx].num = cards[i].num;
			counter[idx].suit[0] = cards[i].suit;
			counter[idx].cnt = 1;
		} else {
			counter[idx].suit[counter[idx].cnt] = cards[i].suit;
			counter[idx].cnt++;
		}
	}

	memset(care_data, 0, sizeof(care_data_t) * 2);
	for (j = 0, i = SUIT_CARDS - 1; i >= 0; i--) {
		if (counter[i].num != 255 && counter[i].cnt >= min ) {
			memcpy(&care_data[j], &counter[i], sizeof(care_data_t));
			j++;	
		}
	}
	
	return j;
}

void init_suits(uint8_t *suits, care_data_t *care_data) {
	int j;

	memset(suits, 255, sizeof(uint8_t) * 4);
	for (j = 0; j < 3; j++) {
		if (care_data->suit[j] == 255)
			continue;

		suits[care_data->suit[j]] = 1;
	}
}

uint16_t care_cc(card_t *cards, uint8_t context) {
	int i, j;
	uint8_t care_data_count;
	care_data_t care_data[4];
	combination_t *combination;
	card_t card0, card1;
	uint8_t idx0, idx1;
	uint16_t cc = 0;
	uint8_t suits[4];
	uint8_t set;
	uint8_t kicker;


	//for (i = 0; i < SUIT_CARDS; i++)
	//	counter[i].num =  255;
	care_data_count = fill_care_data(cards, care_data, 0);

/*	care_data_count = 0;
	memset(care_data, 0, sizeof(care_data_t) * 2);
	for (j = 0, i = SUIT_CARDS - 1; i >= 0; i--) {
		if (counter[i].num != 255 && counter[i].cnt >= 2 ) {
			memcpy(&care_data[j], &counter[i], sizeof(care_data_t));
			j++;	
		}
	}*/

	
//	care_data_count = j;
	if (!care_data_count)
		return 0;

	//printf("coooo=%d\n",care_data_count);
		
	for (i = 0; i <4; i++) {
		if (i == care_data_count)
			break;

		kicker = (i == 0) ? care_data[1].num : care_data[0].num;
		combination = get_combination(COMB_CARE, care_data[i].num);
		combination->kicker[0] = kicker;
		if (care_data[i].cnt == 4) {
			//cc = set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination);
			cc = set_matrix_square(combination);
			return cc;
		} else if (care_data[i].cnt == 3) {
			card0.num = care_data[i].num;

		/*	memset(suits, 255, sizeof(uint8_t) * 4);
			for (j = 0; j < 3; j++) {
				printf("set %d, %d\n", j, care_data[i].suit[j]);
				suits[care_data[i].suit[j]] = 1;
			}*/
			init_suits(suits, &care_data[i]);
			
			for (j = 0; j < 4; j++) {
				if (suits[j] == 255) {
					card0.suit = j;
					break;
				}
			}

			idx0 = card_idx(&card0);
			//printf(" Looking for %u-%u\n", card0.num, card0.suit);
			//cc += set_matrix(idx0, MATRIX_WHOLE, combination);
			cc += set_matrix_line(idx0, combination);
		} else if (care_data[i].cnt == 2) {
			card0.num = card1.num = care_data[i].num;

			/*memset(suits, 255, sizeof(uint8_t) * 4);
			for (j = 0; j < 3; j++) {
				printf("set %d, %d\n", j, care_data[i].suit[j]);
				suits[care_data[i].suit[j]] = 1;
			}*/
			init_suits(suits, &care_data[i]);

			set = 0;
			for (j = 0; j < 4; j++) {
			//	printf("j=%x sj=%x\n",j,suits[j]);
				if (suits[j] == 255) {
					if (set) {
						card1.suit = j;
						break;
					} else {
						card0.suit = j;
						set = 1;
					}
				}
			}

		//	printf(" Looking for %u-%u and %u=%u k=%u\n", card0.num, card0.suit, card1.num, card1.suit, combination->kicker[0]);
			idx0 = card_idx(&card0);
			idx1 = card_idx(&card1);
		//	printf("i %u %u\n",idx0,idx1);
			cc += set_matrix(idx0, idx1, combination);
			
		}
/*
		printf("CARE%d \n",i);
		for (j = 0; j < 3; j++) {
			if (care_data[i].suit[j] == 255)
				continue;
	
			printf("C1=%u-%u cnt=%u\n",care_data[i].num,care_data[i].suit[j],care_data[i].cnt);
		}
*/
	}

	return cc;

}

uint16_t straight_cc(card_t *cards, uint8_t context) {
	int i;
	uint8_t valid_cards[5];

	for (i = 0; i < 5; i++) {
		valid_cards[i] = cards[i].num;
	}

	return straight_common(valid_cards, SUIT_ANY, 5, COMB_STRAIGHT);
}

uint8_t find_best_suit_count(card_t *cards, uint8_t *best_count) {
	uint8_t counter[4], best_suit;
	int i;

	counter[SUIT_PIKI] = 0;
	counter[SUIT_TREFI] = 0;
	counter[SUIT_BUBI] = 0;
	counter[SUIT_CHERVI] = 0;

	*best_count = 0;
	/* detect best suit */
	for (i = 0; i < 5; i++) {
		counter[cards[i].suit]++;

		if (*best_count < counter[cards[i].suit]) {
			best_suit = cards[i].suit;
			(*best_count)++;
		}
	}

	return best_suit;	
}

uint16_t straight_flush_cc(card_t *cards, uint8_t context) {
	uint8_t best_suit, best_count;
	uint8_t valid_cards[5];
	int i, j;
	uint16_t cc = 0;

	best_suit = find_best_suit_count(cards, &best_count);

	if (best_count < 3)
		return 0;

	j = 0;
	for (i = 0; i < 5; i++) {
		if (cards[i].suit == best_suit) 
			valid_cards[j++] = cards[i].num;
	}

	return straight_common(valid_cards, best_suit, best_count, COMB_STRAIGHT_FLUSH);
}

uint16_t straight_common(uint8_t *valid_cards, uint8_t best_suit, uint8_t best_count, uint8_t type) {
	uint8_t needed[2];
	uint8_t need_idx[SUIT_CARDS];
	int i, j;
	uint8_t straight[5], holes, st_idx, diff;
	uint16_t cc = 0;

	simple_sort(valid_cards, best_count);
	
//	for (i = 0; i < best_count; i++) {
//		printf("nstr=%d bc=%d\n", valid_cards[i],best_count);
//	}	


	memset(need_idx, 0, sizeof(uint8_t) * SUIT_CARDS);

	for (i = 0; i < best_count; i++) {
		need_idx[valid_cards[i] - 2] = 1;
	}

	j = holes = 0;
	st_idx = SUIT_CARDS - 1;

	needed[0] = needed[1] = 0;
	for (i = st_idx; i >= 0 ;) {
	//	printf("i=%i, st_idx=%d, j=%d, total=%d\n", i,st_idx,j,SUIT_CARDS);
	//	printf("st=%d St:", st_cards[i]); 


		if (st_idx< 4) {
		//	printf("No str possible, i=%d %d break\n",i, st_idx);
			break;
		}

		if (!need_idx[i]) {
			needed[holes++] = i + 2;
		}

		straight[j] = i + 2;

	/*	int p;
		for (p = 0; p <j+1;p++) {
			printf("%u ", straight[p]);
		}
		printf("\n");*/


		if (j) {
			diff = straight[j - 1] - straight[j];
			if (diff - 1 + holes > 2)  {
			//	printf("too many holes, diff-1=%d, holes=%d switch", diff-1,holes);
				i = --st_idx;
			
				needed[0] = needed[1] = holes = j = 0;
				continue;
			} 

			holes += diff - 1;
		}

		if (j == 4) {
			if (holes < 3) {
	//			printf("saving straight with %d holes %u-%u max=%u\n", holes, needed[0],needed[1],straight[0]);
				cc += save_straight(straight[0], holes, best_suit, needed, type);
			}

			i = --st_idx;
			needed[0] = needed[1] = holes = j = 0;
			continue;
		}
	
		i--;
		j++;
	}

	// special case Ace to five
	holes = 0;
	for (i = 0, j = 0; i < 4; i++) {
		if (!need_idx[i]) {
			holes++;
			needed[j++] = i + 2;
			if (holes > 2) 
				return cc;
		}  
	}

	if (!need_idx[SUIT_CARDS-1]) {
		holes++;
		needed[j++] = CARD_ACE;	
	}

	if (holes > 2)
		return cc;

//	printf("saving ACE straight with %d holes %u-%u\n", holes, needed[0],needed[1]);

	cc += save_straight(5, holes, best_suit, needed, type);
//	cc += save_straight

	return cc;

}


uint16_t royal_flush_cc(card_t *cards, uint8_t context) {
	uint8_t idx0, idx1;
	uint8_t best_suit, best_count = 0;
	uint8_t counter[4];
	uint8_t valid_cards[5], straight[5], needed[2];
	combination_t *combination;
	card_t card;
	card_t *my;
	int i, j;
	uint16_t cc;

	counter[SUIT_PIKI] = 0;
	counter[SUIT_TREFI] = 0;
	counter[SUIT_BUBI] = 0;
	counter[SUIT_CHERVI] = 0;

	if (context == OPPCARDS_CONTEXT) {
		my = cards;
		cards += 2;
	}

	/* detect best suit */
	for (i = 0; i < 5; i++) {
		if (cards[i].num > 9) {
			counter[cards[i].suit]++;
		}
	
		if (best_count < counter[cards[i].suit] && cards[i].num > 9) {
			best_suit = cards[i].suit;
			best_count++;
		}
	}
	
	//best_suit = find_best_suit_count(cards, &best_count);

	if (best_count < 3)
		return 0;

	combination = get_combination(COMB_ROYAL_FLUSH, CARD_ACE);
	if (best_count == 5) {
		cc = set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination);
		return cc;
	}

	j = 0;
	memset(valid_cards, 0, sizeof(uint8_t) * 5);
	for (i = 0; i < 5; i++) {
		if (cards[i].suit == best_suit && cards[i].num > 9) 
			valid_cards[j++] = cards[i].num;
	}

	simple_sort(valid_cards, best_count);

/*	for (i = 0; i < best_count; i++) {
		printf("n=%d\n", valid_cards[i]);
	}	*/

	for (i = 0; i < 5; i++) 
		straight[i] = 0;

	for (i = 0; i < 5; i++) {
		straight[valid_cards[i] - 10] = 1;
	}

	needed[0] = needed[1] = 0;
	for (j = 0, i = 0; i < 5; i++) {
		if (!straight[i]) {
			needed[j++] = i + 10;
		}
	}
	

	card.num = needed[0];
	card.suit = best_suit;
	idx0 = card_idx(&card);


	if (best_count == 4) {
		cc = set_matrix(idx0, MATRIX_WHOLE, combination);
	//		prob = (1/47.0) +  (1.0/47.0) ; // zero intersection of probabilities, so just summ
//			cc = 2 * 46;
	} else if (best_count == 3) {
		card.num = needed[1];	
		idx1 = card_idx(&card);
		//	printf("ixx %u %u\n",idx0,idx1);
		cc = set_matrix(idx0, idx1, combination);
//			prob = 2.0 * ( 1.0/ 47.0) * (1.0/46.0); cc=2
	}

	//printf("best=%d couunt=%u %u-%u\n",best_suit,best_count,needed[0],needed[1]);

	return cc;
}



static void display_tr(void) {
	int i,j;

	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < TOTAL_CARDS; j++) {
			if (tr_matrix[i][j] != (void *) TR_COMB_IMPOSSIBLE &&
				tr_matrix[i][j] != TR_COMB_POSSIBLE)
				printf("x ");
			else
				printf("%x ", tr_matrix[i][j]);
		}
		printf("\n");
	}
}
static void display_opp(void) {
	int i,j;

	printf("opp\n");
	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < TOTAL_CARDS; j++) {
			printf("%x ", opp_matrix[i][j]);
		}
		printf("\n");
	}
}

static void init_opp(card_t *cards) {
	int i, j;
	uint8_t idx[2];

	memcpy(opp_matrix, tr_matrix, sizeof(combination_t *) * TOTAL_CARDS * TOTAL_CARDS);

	idx[0] = card_idx(&cards[TURN_CARD]);
	idx[1] = card_idx(&cards[RIVER_CARD]);

	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < 2; j++) {
			opp_matrix[i][idx[j]] = (void *) TR_COMB_IMPOSSIBLE;
			opp_matrix[idx[j]][i] = (void *) TR_COMB_IMPOSSIBLE;
			
		}
	}
}

static void init_tr(card_t *cards) {
	uint8_t idx[7], total;
	int i, j;

	memset(tr_matrix, TR_COMB_POSSIBLE, sizeof(combination_t *) * TOTAL_CARDS * TOTAL_CARDS);

	for (j = 0; j < 7; j++) {
		if (!cards[j].num)
			break;
		idx[j] = card_idx(&cards[j]);
	}

	total = j;
	/* exclude duplicates */
	for (i = 0; i < TOTAL_CARDS; i++) {
		tr_matrix[i][i] = (void *) TR_COMB_IMPOSSIBLE;

		for (j = 0; j < total; j++) {
			tr_matrix[idx[j]][i] = (void *) TR_COMB_IMPOSSIBLE;
			tr_matrix[i][idx[j]] = (void *) TR_COMB_IMPOSSIBLE;
		}
	}


	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < TOTAL_CARDS; j++) {
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) 
				total_possible++;
		}
	}

	//printf("tp=%u\n", total_possible);
}


