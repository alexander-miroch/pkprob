#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ncurses.h>
#include <signal.h>
#include <locale.h>
#include <ctype.h>

#include "main.h"

int rows, cols;
WINDOW *w_main, *w_bottom, *w_top, *w_table, *w_opps;

extern float preflop_odds[][9];

char card_buf[MAX_CARD_BUF];
ctx_t ctx;

void sigint_handler(int sig) {
	printw("CTRC-C pressed, dying...");
	refresh();
	endwin();

	exit(0);
}

void init_system(void) {

	setlocale(LC_CTYPE,"");
	signal(SIGINT, sigint_handler);

	initscr();
	cbreak();
	noecho();

	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);

	w_main = w_bottom = w_top = NULL;
	clear();

	curs_set(0);

	refresh();
	create_wins();
}

void wprint_card(WINDOW *win, card_t *card) {
	char c;

	if (card->num > 10) {
		switch (card->num) {
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
		}
		wprintw(win, "%c", c);
	} else {
		wprintw(win, "%u", card->num);
	}

	if (card->suit != 255) {
		if (card->suit == SUIT_BUBI || card->suit == SUIT_CHERVI) {
			wattron(win, COLOR_PAIR(1));
		}

		wprintw(win, "%s", v_suits[card->suit]);
		wattroff(win, COLOR_PAIR(1));
	}
}

void create_wins(void) {
	getmaxyx(stdscr, rows, cols);

	w_top = newwin(2, cols, 1, 1);
	wrefresh(w_top);

	w_main = newwin(rows - 7, cols, 3, 0);
	box(w_main, 0, 0);
	wrefresh(w_main);

	w_table = derwin(w_main, rows - 20, 40, 2, 2);
	touchwin(w_main);
	wrefresh(w_table);

	w_opps = derwin(w_main, rows - 20, cols - 35, 2, 35);
	touchwin(w_main);
	wrefresh(w_opps);

	w_bottom = newwin(3, cols, rows - 4, 0);
	wrefresh(w_bottom);
}

void print_status(void) {
	int i;
	combination_t *c = &ctx.combination;
	char *vals[16] = {
		"0", "0", "2", "3", "4","5","6", "7","8","9","10","J","Q","K","A"
	};

	if (!ctx.changed)
		return;

	wclear(w_top);
	wprintw(w_top, "Worst combination:");

	wattron(w_top, A_BOLD);
	wprintw(w_top, " %s ", combs[c->type]);
	wattroff(w_top, A_BOLD);

	if (c->high_card) {
		wprintw(w_top, "high ");
		wattron(w_top, A_BOLD);
		wprintw(w_top, "%s, ", vals[c->high_card]);
		wattroff(w_top, A_BOLD);
	}
	if (c->low_card) {
		wprintw(w_top, "low ");
		wattron(w_top, A_BOLD);
		wprintw(w_top, "%s, ", vals[c->low_card]);
		wattroff(w_top, A_BOLD);
	}

	if (c->kicker[0])
		wprintw(w_top, "kickers:");

	wattron(w_top, A_BOLD);
	for (i = 0; i < 3; i++) {
		if (!c->kicker[i]) 
			break;
		
		wprintw(w_top, " %s", vals[c->kicker[i]]);
	}
	wattroff(w_top, A_BOLD);

	wprintw(w_top, "\t\tPlayers at the table: ");
	wattron(w_top, A_BOLD);
	wprintw(w_top, "%u", ctx.players);
	wattroff(w_top, A_BOLD);

	if (ctx.folders)
		wprintw(w_top, " folded: %u", ctx.folders);

	wrefresh(w_top);
}


void echo_input(card_t *cards, char *text, int num) {
	int i, error = 0;

	memset(cards, 0, num * sizeof(card_t));
	if (parse_buf(cards, num) < 0) {
		wattron(w_bottom, COLOR_PAIR(1));
		error = 1;	
	}

	wprintw(w_bottom, text);
	wattroff(w_bottom, COLOR_PAIR(1));

	for (i = 0; i < num && cards[i].num; i++) {
		wprint_card(w_bottom, &cards[i]);			
	}	

	if (cards[num - 1].num && cards[num - 1].suit != 255) {
		ctx.stop = 1;
		return;
	} else {
		ctx.stop = 0;
	}

	if (error) {
		ctx.stop = 1;
		wprintw(w_bottom, ".");
	} else 	
		ctx.stop = 0;

}

void print_bottom(void) {
	char *text, c;
	card_t cards[7];
	int i, error = 0;

	wclear(w_bottom);
	switch (ctx.state) {
		case ST_WAIT_FOR_PREFLOP:
			echo_input(cards, "Enter your cards: ", 2);
			break;
		case ST_WAIT_FOR_FLOP:
			echo_input(cards, "Enter flop cards: ", 3);
			break;
		case ST_WAIT_FOR_TURN:
			echo_input(cards, "Enter turn card: ", 1);
			break;
		case ST_WAIT_FOR_RIVER:
			echo_input(cards, "Enter river card: ", 1);
			break;
		case ST_WAIT_FOR_FINISH:
			wprintw(w_bottom, "Press enter to restart");
			break;
	}

	wprintw(w_bottom, "_");
	wrefresh(w_bottom);
	
}

uint8_t get_suit(char c) {
	switch (c) {
		case 'p':
			return SUIT_PIKI;
		case 'c':
			return SUIT_CHERVI;
		case 'b':
			return SUIT_BUBI;
		case 'k':
		case 't':
			return SUIT_TREFI;
		default:
			return 255;
	}

	return 255;
}

int parse_buf(card_t *cards, uint8_t count) {
	int i, j, num;
	int istate;
	char c;

	istate = ST_PARSE_WAIT_NUM;
	for (i = 0, j = 0; i < ctx.buf_idx && card_buf[i] && j < count; i++) {
		c = card_buf[i];
		cards[j].suit = 255;

		switch (istate) {
			case ST_PARSE_WAIT_NUM:
				if (!isdigit(c) || c > '9' || c < '1') {
					istate = ST_PARSE_WAIT_SUIT;
					switch (c) {
						case 'A':
						case 'a':
							cards[j].num = CARD_ACE;
							break;
						case 'K':
						case 'k':
							cards[j].num = CARD_KING;
							break;
						case 'Q':
						case 'q':
							cards[j].num = CARD_QUEEN;
							break;
						case 'J':
						case 'j':
							cards[j].num = CARD_JACK;
							break;
						default:
							istate = ST_PARSE_ERROR;
							break;
					}
					
					break;
				}
		
				if (c == '1') {
					cards[j].num = 1;
					istate = ST_PARSE_WAIT_ZERO;
					break;
				}
				
				cards[j].num = c - '0';
				istate = ST_PARSE_WAIT_SUIT;
				break;

			case ST_PARSE_WAIT_SUIT:
				if (!isalpha(c)) {
					istate = ST_PARSE_ERROR;
					break;
				}
				cards[j].suit = get_suit(c);
				if (cards[j].suit == 255) {
					istate = ST_PARSE_ERROR;
					break;
				}
			
				j++;
				istate = ST_PARSE_WAIT_NUM;
				break;
			case ST_PARSE_WAIT_ZERO:
				if (c != '0') {
					istate = ST_PARSE_ERROR;
					break;
				}

				cards[j].num = 10;
				istate = ST_PARSE_WAIT_SUIT;
				break;
			case ST_PARSE_ERROR:
				break;
				

		}

	}

	if (istate == ST_PARSE_ERROR)
		return -1;

	return 0;
}

int valid_cards(card_t *cards, uint8_t cnt) {
	uint8_t idx;
	uint8_t idxs[TOTAL_CARDS];
	int i;

	
	memset(idxs, 0, TOTAL_CARDS);
	for (i = 0; i < cnt; i++) {
		if (!cards[i].num) 
			return 0;

		idx = card_idx(&cards[i]);
		if (idxs[idx]) 
			return 0;

		idxs[idx] = 1;
	}
	
	return 1;
}

void process_river(void) {
	ctx.error = 1;

	if (parse_buf(ctx.cards + 6, 1) < 0) 
		return;
	
	if (!valid_cards(ctx.cards, 7))
		return;

	ctx.mprob = ctx.prob = get_river_prob(ctx.cards, &ctx.combination, ctx.players, &ctx.fi);
	get_opp_combs(ctx.cards, &ctx.combination, 5, ctx.cidxs);

	ctx.error = 0;
}


void process_turn(void) {

	ctx.error = 1;

	if (parse_buf(ctx.cards + 5, 1) < 0) 
		return;
	
	if (!valid_cards(ctx.cards, 6))
		return;

	ctx.mprob = ctx.prob = get_turn_prob(ctx.cards, &ctx.combination, ctx.players, &ctx.fi);
	get_opp_combs(ctx.cards, &ctx.combination, 4, ctx.cidxs);

	ctx.error = 0;
}

void process_flop(void) {

	ctx.error = 1;

	if (parse_buf(ctx.cards + 2, 3) < 0) 
		return;
	
	if (!valid_cards(ctx.cards, 5))
		return;

	ctx.mprob = ctx.prob = get_flop_prob(ctx.cards, &ctx.combination, ctx.players, &ctx.fi);
	get_opp_combs(ctx.cards, &ctx.combination, 3, ctx.cidxs);

	ctx.error = 0;
	
}

void process_preflop(void) {
	float prob;

	ctx.error = 1;
	if (parse_buf(ctx.cards, 2) < 0) 
		return;

	if (!valid_cards(ctx.cards, 2))
		return;

	ctx.error = 0;
	ctx.mprob = ctx.prob = get_pf_probs(ctx.cards, ctx.players, &ctx.fi);
}

void process_folders(void) {
	float prev;

	if (!&ctx.fi.cc)
		return;
	//ctx.mprob = 0;
}

void process_finish(void) {
	free_gc();
	init_ctx();
}

float calc_oprob(float prob, int players) {
//	if (ctx.state == ST_WAIT_FOR_PREFLOP)
//		return 0;

	return (100 - prob) / (players - 1);
}

void process(void) {
	int state;
	float pr;

	ctx.fi.decprob = 0;
	ctx.fi.draw = ctx.fi.drawcc = 0;
	ctx.fi.folders = ctx.folders;
	switch (ctx.state) {
		case ST_WAIT_FOR_PREFLOP:
			process_preflop();
			save_state(ST_PREFLOP);
			state = ST_WAIT_FOR_FLOP;
			break;
		case  ST_WAIT_FOR_FLOP:
			process_flop();
			save_state(ST_FLOP);
			state = ST_WAIT_FOR_TURN;
			break;
		case ST_WAIT_FOR_TURN:
			process_turn();
			save_state(ST_TURN);
			state = ST_WAIT_FOR_RIVER;
			break;
		case ST_WAIT_FOR_RIVER:
			process_river();
			save_state(ST_RIVER);
			state = ST_WAIT_FOR_FINISH;
			break;
		case ST_WAIT_FOR_FINISH:
			process_finish();
			save_state(ST_PREFLOP);
			state = ST_WAIT_FOR_PREFLOP;
			break;
	}

	pr = (ctx.fi.decprob) ? (ctx.mprob + ctx.fi.decprob)/2 : ctx.mprob;
	process_folders();
	ctx.oprob = calc_oprob(pr, ctx.players - ctx.folders);
	if (!ctx.error) {
		ctx.state = state;
		ctx.buf_idx = 0;
	}

}


void print_main(void) {
	int i, j = 0, n;
	card_t cards[7];
	cidx_t *cp;
	card_t card0, card1;	
	float prob;
	
	if (!ctx.changed)
		return;

	wclear(w_table);

	if (ctx.state != ST_WAIT_FOR_PREFLOP) {
		wprintw(w_table, "Table:  ");

		if (ctx.error) 
			wprintw(w_table, "ERROR ");	

		for (i = 2; i < ctx.state + 3; i++) {
			wprint_card(w_table, &ctx.cards[i]);
			wprintw(w_table, " ");
		}

		prob = ctx.fi.decprob ? (ctx.mprob + ctx.fi.decprob)/2 : ctx.mprob;
		wprintw(w_table, "\n\nDraw: %.4g (%.4g %%)\n", ctx.fi.draw * prob, ctx.fi.draw * 100);

		wprintw(w_table, "\nU: ");
		wprint_card(w_table, &ctx.cards[MY_CARD1]);
		wprint_card(w_table, &ctx.cards[MY_CARD2]);
		if (ctx.fi.decprob) {
			wattron(w_table, A_BOLD);
			wprintw(w_table, " \t%.4g", prob);
			wattroff(w_table, A_BOLD);
			wprintw(w_table, " %.4g..%.4g", ctx.mprob, ctx.fi.decprob);
		} else {
			wattron(w_table, A_BOLD);
			wprintw(w_table, " \t%.4g", prob);
			wattroff(w_table, A_BOLD);
		}
	} else {
		wprintw(w_table, "\n\n\n\nU: ");
		if (ctx.error)
			wprintw(w_table, "err");
		else
			wprintw(w_table, "* *   ");
	}


	for (i = 1; i < ctx.players; i++) {
		if (i >= ctx.players - ctx.folders) {
			mvwprintw(w_table, i + 4, 0, "%u: fold   ", i);
			continue;
		}
		mvwprintw(w_table, i + 4, 0, "%u: * *   ", i);
		if (ctx.oprob)
			wprintw(w_table, "\t%.4g", ctx.oprob);
	
	}


	wclear(w_opps);
	wprintw(w_opps, "Combs that opponents can have now\n\n");
	for (i = COMB_ROYAL_FLUSH; i >= 0; i--, j++) {
		cp = &ctx.cidxs[i];

		wprintw(w_opps, "%s ", combs[i]);
		wattron(w_opps, A_BOLD);
		wprintw(w_opps, "%d: ", cp->cnt);
		wattroff(w_opps, A_BOLD);
		for (n = 0; n < cp->cnt; n++) {
			card0.num = card_num(cp->idxs[n].idx0);	
			card0.suit = card_suit(cp->idxs[n].idx0);	
			card1.num = card_num(cp->idxs[n].idx1);	
			card1.suit = card_suit(cp->idxs[n].idx1);	
			wprint_card(w_opps, &card0);
			wprint_card(w_opps, &card1);
	
			if (n < cp->cnt - 1)
				wprintw(w_opps,", ");
		}	
		wprintw(w_opps, "\n\n");
	}

	touchwin(w_main);
	wrefresh(w_table);
	wrefresh(w_opps);
	wrefresh(w_main);

}

void save_players(uint8_t players) {
	FILE *fs;

	fs = fopen(PL_FILE, "w+");
	if (!fs)
		return;

	fprintf(fs, "%hhu", players);
	fclose(fs);
}

void save_state(uint8_t state) {
	FILE *fs;

	fs = fopen(ST_FILE, "w+");
	if (!fs)
		return;

	fprintf(fs, "%hhu", state);
	fclose(fs);
}

uint8_t get_players(void) {
	FILE *fs;
	uint8_t players;
	int rv;

	fs = fopen(PL_FILE, "r");
	if (!fs)
		return 2;

	rv = fscanf(fs, "%hhu", &players);
	if (!rv)
		return 2;

	fclose(fs);
	if (players < 2 || players > MAX_PLAYERS)
		return 2;

	return players;
}

void init_ctx(void) {
	memset(card_buf, 0, MAX_CARD_BUF);
	memset(ctx.cards, 0, sizeof(card_t) * 7);
	memset(&ctx.combination, 0, sizeof(combination_t));
	memset(ctx.cidxs, 255, sizeof(cidx_t) * (MAX_COMB + 1));
	ctx.buf_idx = 0;

	ctx.players = get_players();
	ctx.state = ST_WAIT_FOR_PREFLOP;
	ctx.folders = 0;
	ctx.oprob = 0;

	ctx.error = ctx.stop = 0;
	ctx.bs = ctx.changed = 1;

	ctx.fi.draw = ctx.fi.drawcc = ctx.fi.cc = ctx.fi.decprob = 0;
}

void main_loop(void) {
	uint32_t ch;
	card_t card;
	int bs;

	init_system();
	init_ctx();

	print_bottom();
	save_state(ST_PREFLOP);
	while (1) {
		print_status();
		print_main();
		print_bottom();

		ch = getch();
		ctx.bs = ctx.changed = 0;
		switch (ch) {
			case 43:
				if (ctx.players != MAX_PLAYERS)
					ctx.players++;

				//ctx.oprob = calc_oprob(ctx.mprob, ctx.players);
				save_players(ctx.players);			
				ctx.changed = 1;
				continue;
			case 45:
				if (ctx.players != 2) 
					ctx.players--;
	
				//ctx.oprob = calc_oprob(ctx.mprob, ctx.players);
				save_players(ctx.players);			
				ctx.changed = 1;
				continue;
			case 102:
				if (ctx.folders != ctx.players - 1)
					ctx.folders++;

				process_folders();
				ctx.changed = 1;
				continue;
			case 118:
				if (ctx.folders != 0)
					ctx.folders--;

				process_folders();
				ctx.changed = 1;
				continue;
			case 127:
				if (!ctx.buf_idx)
					continue;
				ctx.buf_idx--;
				card_buf[ctx.buf_idx] = 0;
				ctx.bs = 1;
				break;		
			case 10:
				process();
				ctx.changed = 1;
				break;
		}

		if (ctx.buf_idx < MAX_CARD_BUF && !ctx.bs && !ctx.stop) {
			card_buf[ctx.buf_idx++] = ch;
		}
	}
}
