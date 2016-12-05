#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#include "main.h"

gc_t gc;
uint8_t high_card;
float o_prob_add, o_decprob_add, o_draw_add;
uint32_t o_cc_add, o_drawcc_add;

combination_t *my_matrix[TOTAL_CARDS][TOTAL_CARDS];
extern float preflop_odds[][9];

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

int inline comb_compare(combination_t *a, combination_t *b) {
	int i;

	if (a->type < b->type)
		return -1;

	if (a->type > b->type)
		return 1;

	if (a->high_card < b->high_card)
		return -1;

	if (a->high_card > b->high_card)
		return 1;

	if (a->low_card) {
		if (a->low_card < b->low_card)
			return -1;

		if (a->low_card > b->low_card)
			return 1;
	}

	for (i = 0; i < 3; i++) {
		if (!a->kicker[i])
			return 0;

		if (a->kicker[i] < b->kicker[i])
			return -1;

		if (a->kicker[i] > b->kicker[i])
			return 1;
	}

	return 0;
}

combination_t *get_lowest(uint8_t idx, combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	int i, j, l;
	combination_t *combination, *saved;
	int stop = 0;

	saved = NULL;

	for (i = 0; i < TOTAL_CARDS; i++) {
		stop = 0;
		for (j = 0; j < i && !stop; j++) {
			if (idx != MATRIX_WHOLE) {
				j = idx;
				stop = 1;
			}

			if (tr_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				printf("Something goes wrong...\n");
				exit(1);
			}

			combination = (combination_t *) tr_matrix[i][j];
			if (!saved)
				saved = combination;

			if (comb_compare(combination, saved) < 0) 
				saved = combination;
			/*
			if (combination->type < saved->type) {
				saved = combination;
				continue;
			}
			
			if (combination->type == saved->type) {
				if (combination->high_card < saved->high_card) { 
					saved = combination;
					continue;
				} else if (combination->high_card > saved->high_card) {
					continue;
				}	

				if (combination->low_card < saved->low_card) {
					saved = combination;
					continue;
				} else if (combination->low_card > saved->low_card) {
					continue;
				}

				for (l = 0; l < 3; l++) {
					if (!combination->kicker[l])
						break;

					if (combination->kicker[l] == saved->kicker[l]) {
						continue;
					}

					if (combination->kicker[l] < saved->kicker[l]) {
						saved = combination;
					} else {
						break;
					}

					
				}

			}*/
		}
	}

	return saved;
}

void *tf(void *arg) {
	float o_prob = 0;
	int i,j;
	tha_t *tha = (tha_t *) arg;
	combination_t *combination;
	card_t *cards;
	int players;
	fi_t fi;
	
	fi.decprob = fi.cc = fi.draw = fi.drawcc = 0;
	players = tha->players;
	cards = tha->cards;
	fi.folders = tha->folders;
	for (i = 32; i < TOTAL_CARDS; i++) {
		for (j = 0; j < i; j++) {

			if (my_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (my_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				printf("Something goes wrong...x\n");
				return NULL;
			}
			
			combination = (combination_t *) my_matrix[i][j];

			cards[TURN_CARD].num = card_num(i);
			cards[TURN_CARD].suit = card_suit(i);
			cards[RIVER_CARD].num = card_num(j);
			cards[RIVER_CARD].suit = card_suit(j);
		
			o_prob += get_opponent_win_probability(cards, players, combination, &fi);
		}
	}

	o_prob_add = o_prob;
	o_decprob_add = fi.decprob;
	o_cc_add = fi.cc;
	o_draw_add = fi.draw;
	o_drawcc_add = fi.drawcc;

	return NULL;
}


int main(int argc, char *argv[]) {
/*	card_t cards[2];

	cards[0].num = 7;
	cards[0].suit = SUIT_BUBI;

	cards[1].num = 3;
	cards[1].suit = SUIT_BUBI;

	get_pf_probs(cards);
*/
/*
	card_t c[7];

	c[0].num = CARD_JACK;
	c[0].suit = SUIT_CHERVI;
	c[1].num = 10;
	c[1].suit = SUIT_BUBI;

	c[2].num = c[3].num = 0;
	init_tr(my_matrix, c);

	c[2].num = 4;
	c[2].suit = SUIT_PIKI;
	c[3].num = 5;
	c[3].suit = SUIT_PIKI;
	c[4].num = 2;
	c[4].suit = SUIT_BUBI;
	c[5].num = CARD_ACE;
	c[5].suit = SUIT_TREFI;
	c[6].num = 8;
	c[6].suit = SUIT_TREFI;

	float p;
	set_combs();

	uint16_t cc;
	int i;
	for (i = COMB_ROYAL_FLUSH; i >= 0; i--) {
		cc = ((uint16_t (*)(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS])) (comb_func[i]))(c, 5, my_matrix);
	}
	
	
	combination_t cb;	
	fi_t fi;

	p = get_river_prob(c , &cb, 3, &fi);

	printf("p=%g\n",p);
	
	exit(0);
*/

	char c;

	while ((c = getopt(argc, argv, "d")) != -1) {
		switch (c) {
			case 'd':
				helper_loop();
				exit(0);
				break;

			default:
				break;
		}
	}


	gc.cnt = 0;
	pthread_mutex_init(&gc.mtx, NULL);
	main_loop();

}



float get_pf_probs(card_t *cards, uint8_t players, fi_t *fi) {
	uint16_t idx, diff, parent_offset, child_offset, a0,an;
	uint8_t parent, child, suited, min_players;
	
	if (cards[MY_CARD1].num >= cards[MY_CARD2].num) {
		parent = cards[MY_CARD1].num;
		child = cards[MY_CARD2].num;
	} else {
		parent = cards[MY_CARD2].num;
		child = cards[MY_CARD1].num;
	}
	suited = (cards[MY_CARD1].suit == cards[MY_CARD2].suit) ? 1 : 0;
	
	diff = CARD_ACE - parent;
	an = CARD_ACE * 2 - 3;
	a0 = (parent + 1) * 2 - 3;

	parent_offset = diff * ((an + a0)/2) ;

	diff = parent - child;
	child_offset = diff * 2 - 1;

	if (!suited)
		child_offset++;
		
	parent_offset += child_offset;

	min_players = players - fi->folders;
	if (min_players != players)
		fi->decprob = preflop_odds[parent_offset][min_players - 2];

	return preflop_odds[parent_offset][players - 2];
}

void get_opp_combs(card_t *cards, combination_t *combination, int cnt, cidx_t *cidxs) {
	combination_t *opp_matrix[TOTAL_CARDS][TOTAL_CARDS];
	combination_t *opp_comb;
	uint16_t cc;
	int i, m, n;
	uint8_t num0, num1;
	cidx_t *cp;

	init_tr(opp_matrix, cards);
	memset(cidxs, 255, sizeof(cidx_t) * (MAX_COMB + 1));
	for (i = COMB_ROYAL_FLUSH; i >= combination->type; i--) {
		cc = ((uint16_t (*)(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS])) (comb_func[i]))(cards + 2, cnt, opp_matrix);
		cp = &cidxs[i];
		cp->cnt = 0;
	}


	for (m = 0; m < TOTAL_CARDS; m++) {
		for (n = 0; n < m; n++) {
			if (opp_matrix[m][n] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (opp_matrix[m][n] == (void *) TR_COMB_POSSIBLE) 
				continue;

			opp_comb = (combination_t *) opp_matrix[m][n];

				/* My comb is greater */
			//if (comb_compare(combination, opp_comb) > 0) 
			if (comb_compare(combination, opp_comb) >= 0) 
				continue;
			
			cp = &cidxs[opp_comb->type];
			
			cp->idxs[cp->cnt].idx0 = m;
			cp->idxs[cp->cnt].idx1 = n;

			cp->cnt++;
		}
	}

}
float get_flop_prob(card_t *cards, combination_t *cr, int players, fi_t *fi) {
	float o_prob = 0;
	int i,j;
	uint16_t cc;
	combination_t *combination;
	unsigned char ch[3];
	uint8_t idx, idxr;
	combination_t *opp_matrix[TOTAL_CARDS][TOTAL_CARDS];
	card_t cards2[7];
	pthread_t pthread;
	tha_t tha;

	set_combs();

	init_tr(my_matrix, cards);
	for (i = COMB_ROYAL_FLUSH; i >= 0; i--) {
		cc = ((uint16_t (*)(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS])) (comb_func[i]))(cards, 5, my_matrix);
	}

	combination = get_lowest(MATRIX_WHOLE, my_matrix);
	memcpy(cr, combination, sizeof(combination_t));

	//printf("cr=%u\n",cr->type);

	memcpy(cards2, cards, sizeof(card_t) * 7);
	tha.players = players;
	tha.cards = cards2;
	tha.folders = fi->folders;

	pthread_create(&pthread, NULL, tf, &tha);
	for (i = 0; i < 32; i++) {
		for (j = 0; j < i; j++) {

			if (my_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (my_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				printf("Something goes wrong...\n");
				return -1;
			}
			
			
			combination = (combination_t *) my_matrix[i][j];

			cards[TURN_CARD].num = card_num(i);
			cards[TURN_CARD].suit = card_suit(i);
			cards[RIVER_CARD].num = card_num(j);
			cards[RIVER_CARD].suit = card_suit(j);
		
			o_prob += get_opponent_win_probability(cards, players, combination, fi);
		}
	}

	pthread_join(pthread, NULL);

	fi->drawcc += o_drawcc_add;
	fi->draw += o_draw_add;
	fi->draw = fi->draw / fi->drawcc;

	o_prob += o_prob_add;
	o_prob *= 2;
	o_prob /= 2162;

	fi->cc += o_cc_add;
	fi->decprob += o_decprob_add;
	fi->decprob *= 2;
	fi->decprob /= 2162;

	if (fi->decprob)
		fi->decprob =  (1 - fi->decprob) * 100;

	return (1 - o_prob) * 100;
}

float get_turn_prob(card_t *cards, combination_t *cr, int players, fi_t *fi) {
	float o_prob = 0;
	uint8_t idx;
	int i;
	combination_t *combination;
	
	idx = card_idx(&cards[TURN_CARD]);
	for (i = 0; i < TOTAL_CARDS; i++) {
		if (my_matrix[idx][i] == (void *) TR_COMB_IMPOSSIBLE)
			continue;

		if (my_matrix[idx][i] == (void *) TR_COMB_POSSIBLE) {
			printf("Something goes wrong...2\n");
			exit(1);
		}

	//	printf("processing; ");
	//	print_card(i);
	//	printf("\n");

		cards[RIVER_CARD].num = card_num(i);
		cards[RIVER_CARD].suit = card_suit(i);

		combination = (combination_t *) my_matrix[idx][i];
		o_prob += get_opponent_win_probability(cards, players, combination, fi);
	}

	combination = get_lowest(idx, my_matrix);
	memcpy(cr, combination, sizeof(combination_t));
	//printf("Your combination after turn: ");
	//print_item(2, 2, 0, combination);
	fi->draw = fi->draw / fi->drawcc;

	fi->decprob /= 46;
	if (fi->decprob)
		fi->decprob = ( 1 - fi->decprob ) * 100;

	o_prob /= 46;

	//printf("xxx=%.4f %.4f\n",o_prob,  ( 1 - o_prob) * 100);exit(0);
	return ( 1 - o_prob) * 100;
}


float get_river_prob(card_t *cards, combination_t *cr, int players, fi_t *fi) {
	float o_prob = 0;
	uint8_t idxt, idxr;
	int i;
	combination_t *combination;

	idxt = card_idx(&cards[TURN_CARD]);
	idxr = card_idx(&cards[RIVER_CARD]);

	if (my_matrix[idxt][idxr] == (void *) TR_COMB_IMPOSSIBLE ||
		 my_matrix[idxt][idxr] == (void *) TR_COMB_POSSIBLE) {
		printf("Something goes wrong...\n");
		exit(1);
	}

	combination = (combination_t *) my_matrix[idxt][idxr];
	memcpy(cr, combination, sizeof(combination_t));

//	endwin();
	o_prob = get_opponent_win_probability(cards, players, combination, fi);
	fi->draw = fi->draw / fi->drawcc;

//	printf("ff=%.4f\n",fi->draw);exit(0);

	if (fi->decprob)
		fi->decprob = ( 1 - fi->decprob ) * 100;

	return ( 1 - o_prob ) * 100;
}

void free_gc(void) {
	int i;

	for (i = 0; i < gc.cnt; i++) {
		free(gc.pointer[i]);
	}

	gc.cnt = 0;
}

inline uint16_t fill_third_player(struct cnts_s *ocnts, struct cnts_s *cnts, uint8_t idx0, uint8_t idx1) {
	int i,j;
	uint8_t k = 0;
	uint16_t skipped = 0;

	for (i = 0; i < TOTAL_CARDS; i++) {
		cnts[i].num = 0;

		if (i == idx0 || i == idx1) {
			skipped += ocnts[i].neigh_count;
			continue;
		}

		for (j = 0; j < ocnts[i].neigh_count; j++) {
			if (ocnts[i].neigh[j] == idx0 || ocnts[i].neigh[j] == idx1) {
				skipped++;
				continue;
			}

			cnts[i].num++;
			cnts[ocnts[i].neigh[j]].num++;
		}
	}

	return skipped * 2;
}


float get_opponent_win_probability(card_t *cards, uint8_t players, combination_t *combination, fi_t *fi) {
	float prob;
	uint16_t cc = 0, cc2;
	uint32_t vcc = 0, diff, cnt = 0;
	uint64_t sum = 0;
	int i,j,l;
	float v[5], vv[MAX_PLAYERS], sumprob;
	combination_t *(*comb)[52];
	combination_t *opp_comb;
	uint8_t x;
	uint16_t si = 0, si2 = 0;
	struct cnts_s cnts[TOTAL_CARDS], cnts2[TOTAL_CARDS], *cpt;
	float diff0,diff1,rate;
	combination_t *opp_matrix[TOTAL_CARDS][TOTAL_CARDS];
	int when_save;
	int comb_compare_result;

	if (players > MAX_PLAYERS) {
		printf("too much players");
		exit(0);
	} else if (players == 1) {
		printf("play alone?");
		exit(0);
	}

	init_tr(opp_matrix, cards);
	cards += 2;	


	for (i = COMB_ROYAL_FLUSH; i >= combination->type; i--) {
		//printf("i=%u cc=%d\n",i,cc);
		cc += ((uint16_t (*)(card_t *, uint8_t, combination_t *(*)[TOTAL_CARDS])) (comb_func[i]))(cards, 5, opp_matrix);
	}

//	printf("xccc=%d\n",cc);
//	print_item(0,0,0,combination);

	/* fix equal combs */
	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < i; j++) {
			if (opp_matrix[i][j] == (void *) TR_COMB_IMPOSSIBLE)
				continue;
	
			if (opp_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
				continue;
			}
			opp_comb = (combination_t *) opp_matrix[i][j];

	//		r = comb_compare(opp_comb, combination);

	//		if (r < 0) printf("MYWIN\n");
	//		if (r > 0) printf("HEWIN\n");
	//		if (r == 0) printf("DRAW\n");

			//if (comb_compare(opp_comb, combination) < 0) {
	//		print_item(i,j,cc,opp_comb);
	//		printf("rrcc=%u\n",cc);

			comb_compare_result = comb_compare(opp_comb, combination);
			if (comb_compare_result <= 0) {
				opp_matrix[i][j] = opp_matrix[j][i] = (void *) TR_COMB_POSSIBLE;
	//			printf("i win dec cc\n");
				cc -= 2;
			}

			if (!comb_compare_result) {
	//			printf("DRAW!\n");
				fi->draw += 2;	
			}

		}
	}


	fi->drawcc += (1980 - cc);

	if (!cc) 
		return 0;
	

	when_save = players - fi->folders;
	//fi->cc += cc;
	
	//fi->decprob = 0;
	
	v[0] = (float) cc / (float) 1980;
	if (players == 2) {
		return v[0];
	}
	
	if (v[0] == 1) {
		return 1;
	}

	if (when_save == 2) {
		fi->decprob += v[0];
	}

	//printf("cc=%u\n",cc);
	si = 0;
	for (i = 0; i < TOTAL_CARDS; i++) {
		cnts[i].neigh_count = cnts[i].num = 0;
		for (j = 0; j < i; j++) {
			if (opp_matrix[i][j] != (void *) TR_COMB_POSSIBLE) {
				continue;
			}

			cnts[i].neigh[cnts[i].neigh_count++] = j;

			cnts[i].num++;
			cnts[j].num++;
			si++;
		}
	}

	for (i = 0; i < TOTAL_CARDS; i++) {
		sum += cnts[i].num * cnts[i].num ;
	}

	si *= 2;
	if (si)
		vv[1] = (float) ((float) si + (float) 2.0 - ((float) 4.0 * (float)sum)/(float)si) / 1806;
	else
		vv[1] = 0;

	vv[0] = 1 - v[0];

	if (players == 3) {
		sumprob = (float) 1 - vv[0] * vv[1];
		return sumprob;
	}
	
	if (when_save == 3) {
		fi->decprob += (float) 1 - vv[0] * vv[1];
	}

	vv[2] = v[2] = 0;
	for (i = 0; i < TOTAL_CARDS; i++) {
		cpt = &cnts[i];

		for (j = 0; j < cpt->neigh_count; j++) {
			x = fill_third_player(cnts, cnts2, i, cpt->neigh[j]);
			sum = 0;
		
			for (l = 0; l  < TOTAL_CARDS; l++) {
				sum += cnts2[l].num * cnts2[l].num;
			}
			cc2 = si - x;

			if (cc2) {
				vv[2] += (float) ((float) cc2 + (float) 2.0 - ((float) 4.0 * (float)sum)/(float)cc2);
			}
			si2++;
		}
	}

	if (!si2) {
		vv[2] = 1;
	} else {
		vv[2] /= (float) si2;
		vv[2] /= (float) 1640;
	}

	if (players == 4) {
		sumprob = (float)1 - vv[0] * vv[1] * vv[2];
		return sumprob;
	}

	if (when_save == 4) {
		fi->decprob += (float)1 - vv[0] * vv[1] * vv[2];
	}

	diff0 = vv[0] - vv[1];
	diff1 = vv[1] - vv[2];
	rate = diff1 / diff0;

	vv[3] = vv[2] - diff1 * rate;
	if (players == 5) {
		sumprob = (float)1 - vv[0] * vv[1] * vv[2] * vv[3];
		return sumprob;
	}

	if (when_save == 5) {
		fi->decprob += (float)1 - vv[0] * vv[1] * vv[2] * vv[3];
	}

	vv[4] = vv[3] - ((vv[2] - vv[3]) * rate);
	sumprob = (float)1 - vv[0] * vv[1] * vv[2] * vv[3] * vv[4];
	return sumprob;
/*
	vv[5] = vv[4] - ((vv[3] - vv[4]) * rate);
	if (players == 7) {
		sumprob = (float)1 - vv[0] * vv[1] * vv[2] * vv[3] * vv[4] * vv[5];
		return sumprob;
	}
	
	vv[6] = vv[5] - ((vv[4] - vv[5]) * rate);
	if (players == 8) {
		sumprob = (float)1 - vv[0] * vv[1] * vv[2] * vv[3] * vv[4] * vv[5] * vv[6];
		return sumprob;
	}	


	vv[7] = vv[6] - ((vv[5] - vv[6]) * rate);
	sumprob = (float)1 - vv[0] * vv[1] * vv[2] * vv[3] * vv[4] * vv[5] * vv[6] * vv[7];*/


}

uint16_t set_matrix_single(uint8_t idx0, uint8_t idx1, combination_t *combination, combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	uint16_t cc = 0;
	uint8_t kicker[4];
	combination_t *other;

	kicker[0] = card_num(idx0);
	kicker[1] = card_num(idx1);
	kicker[2] = combination->kicker[0];

	simple_sort(kicker, 3);
	other = get_combination(combination->type, combination->high_card);
	other->kicker[0] = kicker[2];

	cc = set_matrix(idx0, idx1, other, tr_matrix);
	return cc;
}


uint16_t set_matrix_square_none(combination_t *combination,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	uint8_t kicker[7];
	uint16_t cc = 0;
	int i,j;
	combination_t *other;

	for (i = 0; i < TOTAL_CARDS; i++) {
		for (j = 0; j < i; j++) {
			kicker[0] = card_num(i);
			kicker[1] = card_num(j);
			kicker[2] = combination->high_card;
			kicker[3] = combination->low_card;
			kicker[4] = combination->kicker[0];
			kicker[5] = combination->kicker[1];
			kicker[6] = combination->kicker[2];
			simple_sort(kicker, 7);

			other = get_combination(combination->type, kicker[6]);
			other->low_card = kicker[5];
			other->kicker[0] = kicker[4];
			other->kicker[1] = kicker[3];
			other->kicker[2] = kicker[2];
			
			cc += set_matrix(i, j, other, tr_matrix);
		}
	}

	return cc;
}

uint16_t set_matrix_square(combination_t *combination,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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

			cc += set_matrix(i, j, other, tr_matrix);
		}
	}

	return cc;
}

uint16_t set_matrix_line(uint8_t v1, combination_t *combination,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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

		cc += set_matrix(v1, i, other, tr_matrix);
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

uint16_t set_matrix(uint8_t v1, uint8_t v2, combination_t *combination,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	uint32_t cc = 0;
	int i, j;

	if (v1 == MATRIX_WHOLE && v2 == MATRIX_WHOLE) {
		for (i = 0; i < TOTAL_CARDS; i++) {
			for (j = 0; j < i; j++) {
				if (tr_matrix[i][j] == (void *) TR_COMB_POSSIBLE) {
					tr_matrix[i][j] = combination;
					cc++;
				}
	
				if (tr_matrix[j][i] == (void *) TR_COMB_POSSIBLE) {
					tr_matrix[j][i] = combination;
					cc++;
				}
			}
		
		}

		return cc;
	}

	if (v2 == MATRIX_WHOLE) {
		for (i = 0; i < TOTAL_CARDS; i++) {
			if (tr_matrix[i][v1] == (void *) TR_COMB_POSSIBLE) {
				tr_matrix[i][v1] = combination;
				cc++;
			}

			if (tr_matrix[v1][i] == (void *) TR_COMB_POSSIBLE) {
				tr_matrix[v1][i] = combination;
				cc++;
			}
		}		
		return cc;
	} 
	
	if (tr_matrix[v1][v2] == (void *) TR_COMB_POSSIBLE) {
		tr_matrix[v1][v2] = combination;
		cc++;
	}

	if (tr_matrix[v2][v1] == (void *) TR_COMB_POSSIBLE) {
		tr_matrix[v2][v1] = combination;
		cc++;
	}

	return cc;
}

uint16_t save_straight(uint8_t max, uint8_t holes, uint8_t suit, uint8_t *needed, uint8_t type,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	combination_t *combination;
	uint8_t idx0, idx1;
	uint16_t cc = 0;
	card_t card0, card1;
	int i, j;


	combination = get_combination(type, max);
	if (!holes) {
		cc= set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination, tr_matrix);
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
				cc += set_matrix(idx0, MATRIX_WHOLE, combination, tr_matrix);
			}		
	
		} else
			cc = set_matrix(idx0, MATRIX_WHOLE, combination, tr_matrix);

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
				cc += set_matrix(idx0, idx1, combination, tr_matrix);
			}
		}	
	} else 
		cc = set_matrix(idx0, idx1, combination, tr_matrix);

	return cc;
}

void set_double_kickers(combination_t *combination, care_data_t *care_data, uint8_t idx, uint8_t count) {
	switch (idx) {
		case 0:
			combination->kicker[0] = care_data[1].num;
			combination->kicker[1] = care_data[2].num;
			
			combination->kicker[2] = count > 3 ? care_data[3].num : 0;
			break;
		case 1:
			combination->kicker[0] = care_data[0].num;
			combination->kicker[1] = care_data[2].num;
			combination->kicker[2] = count > 3 ? care_data[3].num : 0;
			break;
		case 2:	
			combination->kicker[0] = care_data[0].num;
			combination->kicker[1] = care_data[1].num;
			combination->kicker[2] = count > 3 ? care_data[3].num : 0;
			break;
		case 3:
		case 4:
			combination->kicker[0] = care_data[0].num;
			combination->kicker[1] = care_data[1].num;
			combination->kicker[2] = count > 3 ? care_data[2].num : 0;
			break;
	}
}

uint16_t none_cc(card_t *cards, uint8_t cards_count,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	uint8_t max;
	combination_t *combination;
	uint16_t cc = 0;
	int i;

	combination = get_combination(COMB_NONE, cards[0].num);
	combination->low_card = cards[1].num;
	combination->kicker[0] = cards[2].num;
	if (cards_count > 3) 
		combination->kicker[1] = cards[3].num;
	else
		combination->kicker[1] = 0;
		
	if (cards_count > 4)
		combination->kicker[2] = cards[4].num;
	else
		combination->kicker[2] = 0;

	cc = set_matrix_square_none(combination, tr_matrix);
	
	return cc;
}

uint16_t single_pair_cc(card_t *cards, uint8_t cards_count,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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
	
	care_data_count = fill_care_data(cards, care_data, 0, cards_count);
	if (care_data_count < cards_count - 1)
		return 0;

	for (i = 0; i < care_data_count; i++) {
		if (care_data[i].cnt == 2) {
			combination = get_combination(COMB_SINGLE_PAIR, care_data[i].num);
			set_double_kickers(combination, care_data, i, care_data_count);
			cc = set_matrix_square(combination, tr_matrix);
			return cc;		
		}
	
		combination = get_combination(COMB_SINGLE_PAIR, care_data[i].num);
		set_double_kickers(combination, care_data, i, care_data_count);
	
		card0.num = care_data[i].num;
		init_suits(suits, &care_data[i]);
		for (j = 0; j < 4; j++) {
			if (suits[j] != 255)
				continue;

			card0.suit = j;
			idx0 = card_idx(&card0);

			cc += set_matrix_line(idx0, combination, tr_matrix);
		}
	}


	for (j = CARD_ACE; j >= 2; j--) {
		for (k = 0; k < 4; k++) {
			card0.num = card1.num = j;
			card0.suit = k;
		
			combination = get_combination(COMB_SINGLE_PAIR, card0.num);
			set_double_kickers(combination, care_data, 4, care_data_count);

			idx0 = card_idx(&card0);
			for (l = 0; l < k; l++) {
				if (l == k)
					continue;
					
				card1.suit = l;
				idx1 = card_idx(&card1);
				cc += set_matrix(idx0, idx1, combination, tr_matrix);
			}
		}
	}

	return cc;
}

uint16_t double_pair_cc(card_t *cards, uint8_t cards_count,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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
	uint8_t hc, lc;
	card_t hc0, hc1;
	uint8_t x;

	care_data_count = fill_care_data(cards, care_data, 0, cards_count);
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
					kicker = (j == 1) ? care_data[2].num : care_data[1].num;
				} else
					kicker = care_data[0].num;

				combination->kicker[0] = kicker;						
				init_suits(suits, &care_data[j]);
				if (care_data[j].cnt == 2) {
					for (l = CARD_ACE; l >= 2; l--) {
						if (l == card0.num || l == card1.num)
							continue;

						if (l > card0.num) {
							combination = get_combination(COMB_DOUBLE_PAIR, l);
							combination->low_card = card0.num;
						} else if (l > card1.num) {
							combination = get_combination(COMB_DOUBLE_PAIR, card0.num);
							combination->low_card = l;

						} else 
							continue;

						if (card1.num > kicker) 
							combination->kicker[0] = card1.num;
						else
							combination->kicker[0] = kicker;

						hc0.num = hc1.num = l;
						for (k = 0; k < 4; k++) {
							hc0.suit = k;
							idx0 = card_idx(&hc0);
							for (x = 0; x < k; x++) {
								if (x == k)
									continue;

								hc1.suit = x;
								idx1 = card_idx(&hc1);
								cc += set_matrix(idx0, idx1, combination, tr_matrix);
							}
							
						}
					}
					combination = get_combination(COMB_DOUBLE_PAIR, card0.num);
					combination->low_card = card1.num;
					if (i == 0) {
						kicker = (j == 1) ? care_data[2].num : care_data[1].num;
					} else
						kicker = care_data[0].num;
					combination->kicker[0] = kicker;						

					cc += set_matrix_square(combination, tr_matrix);
				} else if (care_data[j].cnt == 1) {
					for (k = 0; k < 4; k++) {
						if (suits[k] != 255)
							continue;
				
						card1.suit = k;
						idx1 = card_idx(&card1);						
						cc += set_matrix_line(idx1, combination, tr_matrix);
					}
				}
			}

			kicker = (i == 0) ? care_data[1].num : care_data[0].num;

			/* fill any pairs */
			for (j = CARD_ACE; j >= 2; j--) {
				for (k = 0; k < 4; k++) {
					card0.num = card1.num = j;
					card0.suit = k;
	
					if (care_data[i].num > card0.num) {
						hc = care_data[i].num;
						lc = card0.num;
					} else {
						hc = card0.num;
						lc = care_data[i].num;
					}

					combination = get_combination(COMB_DOUBLE_PAIR, hc);
					combination->low_card = lc;
					combination->kicker[0] = kicker;

					idx0 = card_idx(&card0);
					for (l = 0; l < k; l++) {
						if (l == k)
							continue;
					
						card1.suit = l;
						idx1 = card_idx(&card1);
						cc += set_matrix(idx0, idx1, combination, tr_matrix);
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
						cc += set_matrix_line(idx0, combination, tr_matrix);
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
							cc += set_matrix(idx0, idx1, combination, tr_matrix);	
						}
					}
				}

			}


		}

	}
	return cc;
}

uint16_t triplet_cc(card_t *cards, uint8_t cards_count,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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

	care_data_count = fill_care_data(cards, care_data, 0, cards_count);
	if (care_data_count < cards_count - 2)
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
			cc += set_matrix_square(combination, tr_matrix);
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
				cc += set_matrix_line(idx0, combination, tr_matrix);
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
						idx0 = card_idx(&card0);
						idx1 = card_idx(&card1);
						cc += set_matrix(idx0, idx1, combination, tr_matrix);
					} else if (idx == 2) {
						temp_card = card1;
						card1.suit = j;
						idx0 = card_idx(&card0);
						idx1 = card_idx(&card1);
						cc += set_matrix(idx0, idx1, combination, tr_matrix);
						idx0 = card_idx(&temp_card);
						cc += set_matrix(idx0, idx1, combination, tr_matrix);
					}
					idx++;
			}
		}
	}

	return cc;
}

combination_t *get_flush_combination(uint8_t *sorted, uint8_t count) {
	int i, j;
	combination_t *combination;

	combination = get_combination(COMB_FLUSH, sorted[count - 1]);
	combination->low_card = sorted[count - 2];
	for (j = 0, i = count - 3; i >= 0; j++, i--) {
		combination->kicker[j] = sorted[i];
	}
	
	return combination;
}

uint16_t flush_cc(card_t *cards, uint8_t cards_count, combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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

	best_suit = find_best_suit_count(cards, &best_count, cards_count);

	if (best_count < 3)
		return 0;

	j = 0;
	for (i = 0; i < cards_count; i++) {
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
			cc += set_matrix(i, j, combination, tr_matrix);
		}

	}

	return cc;
}

uint16_t full_house_cc(card_t *cards, uint8_t cards_count,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	int i, j, k, l, n;
	uint8_t care_data_count;
	care_data_t care_data[5];
	combination_t *combination;
	card_t card0, card1, temp_card;
	uint8_t idx0, idx1;
	uint8_t cnt, suits[4], suits_supp[4], idx;
	uint16_t cc = 0;

	care_data_count = fill_care_data(cards, care_data, 0, cards_count);

	if (care_data_count == cards_count)
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
						idx0 = card_idx(&card0);
						idx1 = card_idx(&card1);
						cc += set_matrix(idx0, idx1, combination, tr_matrix);
					} else if (idx == 2) {
						temp_card = card1;
						card1.suit = k;

						idx0 = card_idx(&card0);
						idx1 = card_idx(&card1);
						cc += set_matrix(idx0, idx1, combination, tr_matrix);
						idx0 = card_idx(&temp_card);
						cc += set_matrix(idx0, idx1, combination, tr_matrix);
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
						cc += set_matrix(idx0, MATRIX_WHOLE, combination, tr_matrix);
						continue;
					}

					init_suits(suits_supp, &care_data[j]);
					for (l = 0; l < 4; l++) {
						if (suits_supp[l] != 255)
							continue;
						
						card1.suit = l;
						idx1 = card_idx(&card1);
						cc += set_matrix(idx0,idx1, combination, tr_matrix);
					}
				}
			}

		} else if (cnt == 3) {
			for (j = 0; j < care_data_count; j++) {
				if (i == j)
					continue;

////////////////////////

				for (n = CARD_ACE; n > care_data[j].num; n--) {
					for (k = 0; k < 4; k++) {
						card0.num = card1.num = n;
						card0.suit = k;
	
						combination = get_combination(COMB_FULL_HOUSE, care_data[i].num);
						combination->low_card = card0.num;

						idx0 = card_idx(&card0);
						for (l = 0; l < k; l++) {
							if (l == k)
								continue;
					
							card1.suit = l;
							idx1 = card_idx(&card1);
							cc += set_matrix(idx0, idx1, combination, tr_matrix);
						}
					}
				}

////////////////////////


				combination = get_combination(COMB_FULL_HOUSE, care_data[i].num);
				combination->low_card = care_data[j].num;

				if (care_data[j].cnt == 2) {
					cc += set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination, tr_matrix);
					return cc;
				}

				card0.num  = care_data[j].num;
				init_suits(suits, &care_data[j]);

				for (k = 0; k < 4; k++) {
					if (suits[k] != 255)
						continue;

					card0.suit = k;
					idx0 = card_idx(&card0);
					cc += set_matrix(idx0, MATRIX_WHOLE, combination, tr_matrix);
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
						cc += set_matrix(idx0, idx1, combination, tr_matrix);
					}
				}
			}
		}
	}	

	return cc;

}

uint8_t fill_care_data(card_t *cards, care_data_t *care_data, uint8_t min, uint8_t cards_count) {
	int i, j;
	uint8_t idx;
	care_data_t counter[SUIT_CARDS];

	memset(counter, 255, sizeof(care_data_t) * SUIT_CARDS);
	for (i = 0, j = 0; i < cards_count; i++) {
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

uint16_t care_cc(card_t *cards, uint8_t cards_count,  combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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

	care_data_count = fill_care_data(cards, care_data, 0, cards_count);
	if (!care_data_count)
		return 0;

	for (i = 0; i <4; i++) {
		if (i == care_data_count)
			break;

		kicker = (i == 0) ? care_data[1].num : care_data[0].num;
		combination = get_combination(COMB_CARE, care_data[i].num);
		combination->kicker[0] = kicker;
		if (care_data[i].cnt == 4) {
			cc = set_matrix_square(combination, tr_matrix);
			return cc;
		} else if (care_data[i].cnt == 3) {
			card0.num = care_data[i].num;
			init_suits(suits, &care_data[i]);
			
			for (j = 0; j < 4; j++) {
				if (suits[j] == 255) {
					card0.suit = j;
					break;
				}
			}
		
			idx0 = card_idx(&card0);
			cc += set_matrix_line(idx0, combination, tr_matrix);
		} else if (care_data[i].cnt == 2) {
			card0.num = card1.num = care_data[i].num;
			init_suits(suits, &care_data[i]);

			set = 0;
			for (j = 0; j < 4; j++) {
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

			idx0 = card_idx(&card0);
			idx1 = card_idx(&card1);
			cc += set_matrix(idx0, idx1, combination, tr_matrix);
			
		}
	}

	return cc;

}

uint16_t straight_cc(card_t *cards, uint8_t cards_count, combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	int i;
	uint8_t valid_cards[5];

	for (i = 0; i < cards_count; i++) {
		valid_cards[i] = cards[i].num;
	}

	return straight_common(valid_cards, SUIT_ANY, cards_count, COMB_STRAIGHT, tr_matrix);
}

uint8_t find_best_suit_count(card_t *cards, uint8_t *best_count, uint8_t cards_count) {
	uint8_t counter[4], best_suit;
	int i;

	counter[SUIT_PIKI] = 0;
	counter[SUIT_TREFI] = 0;
	counter[SUIT_BUBI] = 0;
	counter[SUIT_CHERVI] = 0;

	*best_count = 0;

	/* detect best suit */
	for (i = 0; i < cards_count; i++) {
		counter[cards[i].suit]++;

		if (*best_count < counter[cards[i].suit]) {
			best_suit = cards[i].suit;
			(*best_count)++;
		}
	}

	return best_suit;	
}

uint16_t straight_flush_cc(card_t *cards, uint8_t cards_count, combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	uint8_t best_suit, best_count;
	uint8_t valid_cards[5];
	int i, j;
	uint16_t cc = 0;

	best_suit = find_best_suit_count(cards, &best_count, cards_count);

	if (best_count < 3)
		return 0;

	j = 0;
	for (i = 0; i < cards_count; i++) {
		if (cards[i].suit == best_suit) 
			valid_cards[j++] = cards[i].num;
	}

	return straight_common(valid_cards, best_suit, best_count, COMB_STRAIGHT_FLUSH, tr_matrix);
}

uint16_t straight_common(uint8_t *valid_cards, uint8_t best_suit, uint8_t best_count, uint8_t type, combination_t *(*tr_matrix)[TOTAL_CARDS]) {
	uint8_t needed[2];
	uint8_t need_idx[SUIT_CARDS];
	int i, j;
	uint8_t straight[5], holes, st_idx, diff;
	uint16_t cc = 0;

	simple_sort(valid_cards, best_count);
	memset(need_idx, 0, sizeof(uint8_t) * SUIT_CARDS);

	for (i = 0; i < best_count; i++) {
		need_idx[valid_cards[i] - 2] = 1;
	}

	j = holes = 0;
	st_idx = SUIT_CARDS - 1;

	needed[0] = needed[1] = 0;
	for (i = st_idx; i >= 0 ;) {
		if (st_idx< 4) {
			break;
		}

		if (!need_idx[i]) {
			needed[holes++] = i + 2;
		}

		straight[j] = i + 2;

		if (j) {
			diff = straight[j - 1] - straight[j];
			if (diff - 1 + holes > 2)  {
				i = --st_idx;
			
				needed[0] = needed[1] = holes = j = 0;
				continue;
			} 

			holes += diff - 1;
		}

		if (j == 4) {
			if (holes < 3) {
				cc += save_straight(straight[0], holes, best_suit, needed, type, tr_matrix);
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

	cc += save_straight(5, holes, best_suit, needed, type, tr_matrix);

	return cc;

}


uint16_t royal_flush_cc(card_t *cards, uint8_t cards_count, combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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

	/* detect best suit */
	for (i = 0; i < cards_count; i++) {
		if (cards[i].num > 9) {
			counter[cards[i].suit]++;
		}
	
		if (best_count < counter[cards[i].suit] && cards[i].num > 9) {
			best_suit = cards[i].suit;
			best_count++;
		}
	}
	
	if (best_count < 3)
		return 0;

	combination = get_combination(COMB_ROYAL_FLUSH, CARD_ACE);
	if (best_count == 5) {
		cc = set_matrix(MATRIX_WHOLE, MATRIX_WHOLE, combination, tr_matrix);
		return cc;
	}

	j = 0;
	memset(valid_cards, 0, sizeof(uint8_t) * 5);
	for (i = 0; i < cards_count; i++) {
		if (cards[i].suit == best_suit && cards[i].num > 9) 
			valid_cards[j++] = cards[i].num;
	}

	simple_sort(valid_cards, best_count);

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
		cc = set_matrix(idx0, MATRIX_WHOLE, combination, tr_matrix);
	} else if (best_count == 3) {
		card.num = needed[1];	
		idx1 = card_idx(&card);
		cc = set_matrix(idx0, idx1, combination, tr_matrix);
	}

	return cc;
}


/*
static void display_tr(combination_t *(*tr_matrix)[TOTAL_CARDS]) {
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
*/

static void init_tr(combination_t *(*tr_matrix)[TOTAL_CARDS], card_t *cards) {
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
}


















///// Players parse ////////////



#define BLANK_KICKER	CARD_QUEEN


int _is_triplet(card_t *cards, uint8_t count, combination_t *combination) {
	uint8_t care_data_count;
	care_data_t care_data[5];

	care_data_count = fill_care_data(cards, care_data, 0, count);
	if (care_data_count < count - 2)
		return 0;

	if (care_data[0].cnt == 3) {
		combination->type = COMB_TRIPLET;
		combination->high_card = care_data[0].num;
		combination->kicker[0] = care_data[1].num;
		
	} else if (care_data[1].cnt == 3) {
		combination->type = COMB_TRIPLET;
		combination->high_card = care_data[1].num;
		combination->kicker[0] = care_data[0].num;
	} else {
		return 0;
	}

	return 1;
}

int _is_care(card_t *cards) {
	int i, num;

	num = cards[0].num;
	for (i = 1; i < 4; i++) {
		if (cards[i].num != num)
			return 0;
	}

	return 1;
}

int _is_double(card_t *cards, combination_t *combination) {
	uint8_t care_data_count;
	care_data_t care_data[5];

	care_data_count = fill_care_data(cards, care_data, 0, 4);
	if (care_data_count != 2)
		return 0;

	combination->type = COMB_DOUBLE_PAIR;
	if (care_data[0].num < care_data[1].num) {
		combination->high_card = care_data[1].num;
		combination->low_card = care_data[0].num;
	} else {
		combination->high_card = care_data[0].num;
		combination->low_card = care_data[1].num;
	}

	return 1;
}

int _is_pair(card_t *cards, uint8_t count, combination_t *combination) {
	uint8_t care_data_count;
	care_data_t care_data[5];
	int i = 0;
	uint8_t num0, num1;
	int find = 0;

	care_data_count = fill_care_data(cards, care_data, 0, count);
	if (care_data_count < count - 1)
		return 0;

	num0 = num1 = 0;
	for (; i < care_data_count; i++) {
		if (care_data[i].cnt == 2) {
			combination->type = COMB_SINGLE_PAIR;
			combination->high_card = care_data[i].num;
			find = 1;
		} else if (care_data[i].cnt == 1) {
			if (!num0)
				num0 = care_data[i].num;
			else
				num1 = care_data[i].num;
		} else {
			return 0;
		}
	}

	if (!find)
		return 0;

	if (num0 < num1) {
		combination->kicker[0] = num1;
		combination->kicker[1] = num0;
	} else {
		combination->kicker[0] = num0;
		combination->kicker[1] = num1;
	}

	if (!num1)
		combination->kicker[1] = BLANK_KICKER;

	combination->kicker[2] = BLANK_KICKER - 1;
	return 1;
}

void set_combination(card_t *cards, combination_t *combination, uint8_t count) {
	if (count == 4 && _is_care(cards)) {
		combination->type = COMB_CARE;
		combination->high_card = cards[0].num;
		combination->low_card = (combination->high_card == BLANK_KICKER) ? BLANK_KICKER - 1 : BLANK_KICKER;
		return;
	}

	if (_is_triplet(cards, count, combination)) {
		if (count == 3) {
			combination->kicker[0] = (combination->high_card == BLANK_KICKER) ? BLANK_KICKER - 1 : BLANK_KICKER;
		}
		return;
	}

	if (count == 4 && _is_double(cards, combination)) {
		combination->kicker[0] = (combination->high_card == BLANK_KICKER) ? BLANK_KICKER - 1 : BLANK_KICKER;
		return;
	}

	if (_is_pair(cards, count, combination)) {
		return;
	}

	combination->type = COMB_NONE;
	return;
}

int __ace_gutshot(uint8_t *nums, uint8_t imp0, uint8_t imp1) {
	int i, n;
	uint8_t idx, counts[4];

	memset(counts, 0, sizeof(uint8_t) * 4);
	for (i = 0; i < 5; i++) {
		if (nums[i] > 5)
			break;

		idx = nums[i] - 2;
		counts[idx] = 1;		
	}

	for (n = 0, i = 0; i < 4; i++) {
		if (!counts[i])
			n++;
	}

	if (n == 1) 
		return 1;

	return 0;
}
int __sub_straight_check(uint8_t *nums, uint8_t count, uint8_t imp0, uint8_t imp1) {
	int i;
	uint8_t prev, diff, saved_asc, saved_base, asc, missed;
	uint8_t our = 0;

	if (count < 4)
		return 0;

	prev = nums[count - 1];
	if (prev == imp0 || prev == imp1)
		our = 1;

	saved_base = missed = asc = saved_asc = 0;
	for (i = count - 2; i >= 0; i--) {
//	printf("px=%u\n", nums[i]);
		diff = prev - nums[i];
		if (!diff)
			continue;

		if (diff == 1) {
			++asc;
			if (nums[i] == imp0 || nums[i] == imp1)
				our = 1;

			if (asc == 3) {
//				printf("our=%u\n",our);
				return  our ? 1 : 0;
			}

			prev = nums[i];
			continue;
		}


		if (diff == 2) {
			if (!missed) {
				asc++;
				if (nums[i] == imp0 || nums[i] == imp1)
					our = 1;

				if (asc == 3) {
					return our ? 1 : 0;
				}

			} else {
				return 0;
			}
			prev = nums[i];
			continue;
		}

		asc = 0;
		prev = nums[i];
		our = 0;

	}

	return 0;
}

int _check_straight_draw(card_t *cards, uint8_t count) {
	uint8_t nums[6], uimp0, uimp1, imp0, imp1, ace_here;
	uint8_t prev, diff, saved_asc, saved_base, asc, missed;
	int i;
	uint8_t our = 0;

	memset(nums, 0, sizeof(uint8_t) * 6);
	for (i = 0; i < count; i++) 
		nums[i] = cards[i].num;
	
	ace_here = imp0 = imp1 = uimp0 = uimp1 = 0;
	for (i = 2; i < count; i++) {
		if (cards[i].num == cards[0].num)
			uimp0 = 1;
		if (cards[i].num == cards[1].num)
			uimp1 = 1;
	}

	for (i = 0; i < count; i++) {
		if (cards[i].num == CARD_ACE)
			ace_here = 1;
	}


	if (!uimp0)
		imp0 = cards[0].num;
	if (!uimp1)
		imp1 = cards[1].num;

	if (uimp0 && uimp1)
		return 0;

	simple_sort(nums, count);
	prev = nums[count - 1];
	if (prev == imp0 || prev == imp1)
		our = 1;
	
//	printf("i=%u %u\n",imp0,imp1);

	saved_base = missed = asc = saved_asc = 0;
	for (i = count - 2; i >= 0; i--) {
//		printf("n=%u\n",nums[i]);
		diff = prev - nums[i];
		if (!diff)
			continue;

		if (diff == 1) {
			++asc;
			if (nums[i] == imp0 || nums[i] == imp1)
				our = 1;

//			printf("asc=%u our=%u\n",asc, our);
			if (asc == 3) {
				if (!our) {
//					printf("not our\n");
					if (__sub_straight_check(nums, saved_base + 1, imp0, imp1)) {
						return RET_GUT_SHOT;
					}

					if (ace_here && __ace_gutshot(nums, imp0, imp1)) {
						return RET_GUT_SHOT;
					}	
					return 0;	
				}			
	
//				printf("sb=%u x=%u\n", saved_base, nums[saved_base]);

				if (missed) {
					/* check for other straight */
					if (__sub_straight_check(nums, saved_base + 1, imp0, imp1)) {
//						printf("here\n");
						return RET_STRAIGHT_DRAW;
					}

					if (ace_here && __ace_gutshot(nums, imp0, imp1)) {
						return RET_STRAIGHT_DRAW;
					}	
		
					return RET_GUT_SHOT;
				}
								

				/* straight A K Q J */
				if (nums[i] == CARD_JACK) {
					return RET_GUT_SHOT;
				} else {
					return RET_STRAIGHT_DRAW;
				}

			
			}


			prev = nums[i];
			continue;
		}


		if (diff == 2) {
			if (!missed) {
				asc++;
				saved_base = i;
				if (nums[i] == imp0 || nums[i] == imp1)
					our = 1;

	//		printf("curr=%u prev=%u our=%u\n", nums[i], prev, our);
				if (asc == 3) {
					if (!our) {
	//					printf("missed not our\n");
						return 0;
					}
		
					return RET_GUT_SHOT;
				}
				missed++;
			} else {
				our = 0;			
				missed = asc = 0;
				prev = i = saved_base;

				if (nums[i] == imp0 || nums[i] == imp1)
					our = 1;

				if (prev == imp0 || prev == imp1)
					our = 1;

			}
			prev = nums[i];
			continue;
		}

		saved_asc = asc = 0;
		prev = nums[i];
		our = 0;

		if (nums[i] == imp0 || nums[i] == imp1)
			our = 1;
	}

//	for (i = 0; i < count; i++) {
//		printf("c=%u\n",nums[i]);
//	}

//	printf("none!\n");
	if (ace_here && __ace_gutshot(nums, imp0, imp1)) {
		return RET_GUT_SHOT;
	}	
	return 0;
}

int _check_flush_draw(card_t *cards, uint8_t count) {
	uint8_t best_suit;
	uint8_t best_count;

	best_suit = find_best_suit_count(cards, &best_count, count);
	if (best_count >= 4) {
		if (cards[0].suit == best_suit ||
			cards[1].suit == best_suit) 
				return RET_FLUSH_DRAW;
	}

	return 0;
}

int _pair_state(card_t *cards, uint8_t count, uint8_t hc) {
	uint8_t care_data_count;
	int i, misses = 0;
	int find = 0, eq = 0;
	care_data_t care_data[5];

	if (cards[0].num != hc && cards[1].num != hc) {
		printf("something realy bad\n");
		exit(0);
	}

	cards += 2;
	care_data_count = fill_care_data(cards, care_data, 0, count - 2);

	for (i = 0; i < care_data_count; i++) {
		if (care_data[i].num > hc) {
			if (++misses >  1) 
				return RET_BOTTOM_PAIR;
		} else if (care_data[i].num == hc) 
			eq = 1;
	}

	if (misses == 1) 
		return RET_MIDDLE_PAIR;


	return eq ? RET_TOP_PAIR : RET_OVER_PAIR;	

}
