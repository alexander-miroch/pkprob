#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ncurses.h>
#include <signal.h>
#include <locale.h>
#include <ctype.h>
#include <mysql.h>

#include "main.h"

MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;

#define DBHOST "localhost"
#define DBNAME "pk"
#define DBUSER "pk"
#define DBPASS "pk"

int rows, cols;
WINDOW *w_main, *w_bottom, *w_top, *w_table;

void init_db(void);

char card_buf[MAX_CARD_BUF];

#define PLAYER_NAME_LEN	256
typedef struct player_s {
	char name[PLAYER_NAME_LEN];
	uint32_t id;
} player_t;

#define NPLAYERS 16
#define QBUF 1024

int get_player_list(player_t *, uint32_t);
#define MODE_INPUT 0x0
#define MODE_SHOW  0x1


typedef struct mcnt_s {
	uint64_t cnt;
	uint64_t fdraw, sdraw, gs;
	uint64_t gs_fdraw, sdraw_fdraw;
} mcnt_t;

#define MAX_COMB2 (COMB_ROYAL_FLUSH + 3)
mcnt_t mcnt[MAX_COMB2];

const char *states[] = {
	"PREFLOP",
	"FLOP",
	"TURN",
	"RIVER"
};

const char *actions[] = {
	"FOLD",
	"CHECK",
	"CALL",
	"RAISE",
	"BET",
	"RERAISE",
	"RECALL"
};


typedef struct pctx_s {
	char name[256];
	uint16_t idx;
	int state, mode, action;
	int partial;
	int changed, bs, stop, need_db_req;
	uint32_t selected_id;
	player_t players[NPLAYERS];
} pctx_t;

pctx_t ctx;

void sigint_handler(int sig) {
	printw("CTRC-C pressed, dying...");
	refresh();
	endwin();
	mysql_close(conn);

	exit(0);
}

void init_system(void) {

	setlocale(LC_CTYPE,"");
	signal(SIGINT, sigint_handler);

	initscr();
	cbreak();
	noecho();

	keypad(stdscr, TRUE);

	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);

	w_main = w_bottom = w_top = NULL;
	clear();

	curs_set(0);

	refresh();
	create_wins();

	init_db();
}

void init_db(void) {
	conn = mysql_init(NULL);
	if (!mysql_real_connect(conn, DBHOST, DBUSER, DBPASS,
			DBNAME, 0, NULL, 0)) {
		endwin();
		fprintf(stderr, "Can't connect to mysql: %s\n", mysql_error(conn));
		exit(1);
	}
}

void wprint_card(WINDOW *win, uint8_t num) {
	char c;

	if (num > 9) {
		switch (num) {
			case 10:
				c = 'T';
				break;
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
		wprintw(win, "%u", num);
	}
}

void create_wins(void) {
	getmaxyx(stdscr, rows, cols);

	w_top = newwin(2, cols, 1, 1);
	wrefresh(w_top);

	w_main = newwin(rows - 7, cols, 3, 0);
	box(w_main, 0, 0);
	wrefresh(w_main);

	w_table = derwin(w_main, rows - 12, cols - 4, 2, 2);
	touchwin(w_main);
	wrefresh(w_table);

	w_bottom = newwin(3, cols, rows - 4, 0);
	wrefresh(w_bottom);
}

void print_bottom(void) {
	char *text, c;
	card_t cards[7];
	int i, error = 0;

	werase(w_bottom);
	wprintw(w_bottom, "Enter player name: ");
	wprintw(w_bottom, ctx.name);

	wprintw(w_bottom, "_");
	wrefresh(w_bottom);
	
}

player_t players[NPLAYERS];
void show_player_list(void) {
	int i;
	char s;

//a	memset(&players, 0, sizeof(player_t) * NPLAYERS);
	if (ctx.need_db_req) {
		memset(&ctx.players, 0, sizeof(player_t) * NPLAYERS);
		ctx.partial = get_player_list(ctx.players, NPLAYERS);
	}
//	partial = get_player_list(players, NPLAYERS);

	werase(w_table);
	wprintw(w_table, "selid=%u\n",ctx.selected_id);
	for (i = 0; i < NPLAYERS; i++) {
	//	if (!ctx.players[i].id)
		if (!ctx.players[i].id)
			break;

		if (!ctx.selected_id)
			ctx.selected_id = ctx.players[i].id;

		s = (ctx.players[i].id == ctx.selected_id) ? '*' : 0x20;
		

		wprintw(w_table, "%c %s\n",s, ctx.players[i].name, ctx.players[i].id);
	}

	if (ctx.partial) {
		wprintw(w_table, "...\n");
	}

	wrefresh(w_table);
	
}

int get_player_list(player_t *ret, uint32_t limit) {
	char buf[PLAYER_NAME_LEN + 60];
	int i = 0;

	snprintf(buf, PLAYER_NAME_LEN + 60, "select id,name from players where name like '%s%%'", ctx.name);
	if (mysql_query(conn, buf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", buf, mysql_error(conn));
		exit(1);
	}

	res = mysql_use_result(conn);
	while ((row = mysql_fetch_row(res)) != NULL) {

		memcpy(ret->name, row[1], strlen(row[1]));
		ret->id = (uint32_t) atoi(row[0]);

		if (i++ == limit - 1) {
			mysql_free_result(res);
			return 1;
		}	
			
		ret++;
	}

	mysql_free_result(res);
	return 0;
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



typedef struct common_s {
	float prob;
	uint32_t count, total_count;
	char action[16];
} common_t;
#define MAX_ACTIONS 7

typedef struct stat_s {
	float vpip, vpip_check_as_act, pfr, c3bet;
	float c3bet_button, pfr_button;
	float af, f2r, chr;
	float cb, db, f2cb;
	float no_folds, wins;
	int common_count;
	common_t common[MAX_ACTIONS];
} stat_t;

stat_t stat;

void show_common(char *, char *, stat_t *);

void inline inc_bool_cnts(char *val, uint64_t *cnt, uint64_t *pos_cnt) {
	if (!strcmp(val, "YES")) {
		(*cnt)++;
	} else if (!strcmp(val, "NO")) {
		(*pos_cnt)++;
	}
}

char inline *wsd_state(char *state) {
	 if (!strcmp(state,"FLOP")) 
		return "'FLOP', 'TURN', 'RIVER'";
	 if (!strcmp(state,"TURN")) 
		return "'TURN', 'RIVER'";
	 if (!strcmp(state,"RIVER")) 
		return "'RIVER'";

	return "NULL";	
}

void get_wsd(stat_t *stat, char *state) {
	char qbuf[QBUF];
	uint64_t games, folds, wins;
	uint64_t no_folds;
	unsigned long rows;
	char *wstate;

	/* how much he is doesn't fold */
	wstate = wsd_state(state);
	snprintf(qbuf, QBUF, "select count(distinct gameid) games, \
			count(case when action='FOLD' then 1 else NULL end) as folds \
			from hands where playerid=%u and state IN (%s)", ctx.selected_id, wstate);

	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}

	res = mysql_store_result(conn);
	rows = mysql_num_rows(res);
	if (rows != 1) {
		endwin();
		fprintf(stderr, "Query %s failed (invalid rows %lu)\n", qbuf, rows);
		exit(1);
	}

	row = mysql_fetch_row(res);
	games = strtoll(row[0], NULL, 10);
	folds = strtoll(row[1], NULL, 10);
	mysql_free_result(res);

	/* how much he wins when he doesn't fold */
	snprintf(qbuf, QBUF, "select count(case when won=1 then 1 else NULL end) as won \
		 from (select distinct gameid, won from hands where playerid=%u and state IN (%s)) s", ctx.selected_id, wstate);
	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}

	res = mysql_store_result(conn);
	rows = mysql_num_rows(res);
	if (rows != 1) {
		endwin();
		fprintf(stderr, "Query %s failed (invalid rows %lu)\n", qbuf, rows);
		exit(1);
	}

	row = mysql_fetch_row(res);
	wins = strtoll(row[0], NULL, 10);
	mysql_free_result(res);

	if (!games)
		return;
	
	no_folds = games - folds;
	stat->no_folds = (float) no_folds / (float) games;
	stat->wins = (float) wins / (float) no_folds;
}

void get_oth_stats(stat_t *stat, char *state) {
	char qbuf[QBUF];
	uint64_t cnt_possible_chr, cnt_chr, total;
	uint64_t cnt_possible_db, cnt_db;
	uint64_t cnt_possible_cb, cnt_cb;
	uint64_t cnt_possible_f2cb, cnt_f2cb;
	uint64_t prevgameid = 0, gameid;
	int checked = 0;

	snprintf(qbuf, QBUF, "select action, gameid, st_donk_bet, st_cb, st_fold_to_cb \
				from hands where playerid=%u AND state='%s' order by gameid, prio", ctx.selected_id, state);
	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}

	cnt_possible_chr = cnt_chr = 0;
	cnt_possible_db = cnt_db = 0;
	cnt_possible_cb = cnt_cb = 0;
	cnt_possible_f2cb = cnt_f2cb = 0;
	res = mysql_use_result(conn);
	while ((row = mysql_fetch_row(res)) != NULL) {
		gameid = strtoll(row[1], NULL, 10);

		/* donkbet */
		inc_bool_cnts(row[2], &cnt_db, &cnt_possible_db);
			
		/* contbet */
		inc_bool_cnts(row[3], &cnt_cb, &cnt_possible_cb);

		/* fold 2contbet */
		inc_bool_cnts(row[4], &cnt_f2cb, &cnt_possible_f2cb);

		if (prevgameid == gameid) {
			if (!checked) 
				continue;
			
			if (!strcmp(row[0], "RAISE") ||
				!strcmp(row[0], "RERAISE")) {
				cnt_chr++;
			} else {
				cnt_possible_chr++;
			}

			checked = 0;
			continue;
		}

		checked = 0;
		if (!strcmp(row[0], "BET")) {
			cnt_possible_chr++;
		} else if (!strcmp(row[0], "CHECK")) {
			checked = 1;
		}


		prevgameid = gameid;
//		printf("s=%s\n",row[0]);
	}

	mysql_free_result(res);
	
	total = cnt_db + cnt_possible_db;	
	if (total)
		stat->db = (float) ((float) cnt_db / (float) total);

//	endwin();
//	printf("cb=%llu pos=%llu\n",cnt_cb, cnt_possible_cb);exit(0);
	
	total = cnt_cb + cnt_possible_cb;
	if (total)
		stat->cb = (float) ((float) cnt_cb / (float) total);

	total = cnt_f2cb + cnt_possible_f2cb;
	if (total)
		stat->f2cb = (float) ((float) cnt_f2cb / (float) total);
		
	total = cnt_possible_chr + cnt_chr;
	if (total)
		stat->chr = (float) ((float) cnt_chr / (float) total);
//	printf("pos=%llu chr=%llu\n",cnt_possible_chr,cnt_chr);

}

void calc_af(stat_t *stat) {
	int i;
	common_t *common; 
	float result;
	uint64_t cnt_act, cnt_pass, total;
	
	cnt_act = cnt_pass = 0;
	for (i = 0; i < stat->common_count; i++) {
		common = &stat->common[i];

		
		if (!strcmp(common->action, "CHECK") ||
			!strcmp(common->action, "FOLD") ||
			!strcmp(common->action, "RECALL") ||
			!strcmp(common->action, "CALL")) {
				cnt_pass += common->total_count;
		} else if (!strcmp(common->action, "BET") ||
			!strcmp(common->action, "RAISE") ||
			!strcmp(common->action, "RERAISE")) {
			cnt_act += common->total_count;
		}
	}

	total = cnt_act + cnt_pass;
	if (!total)
		return;

	stat->af = (float) ((float) cnt_act / (float) total);
}

void get_stats(stat_t *stat, char *state) {
	char qbuf[QBUF];
	uint64_t cnt_f2r_y, cnt_f2r_n, total;


	snprintf(qbuf, QBUF, "select st_f2r from hands where playerid=%u AND state='%s'", ctx.selected_id, state);
	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}

	res = mysql_use_result(conn);
	cnt_f2r_y = cnt_f2r_n = 0;
	while ((row = mysql_fetch_row(res)) != NULL) {
		if (!strcmp(row[0], "YES")) {
			cnt_f2r_y++;
		} else if (!strcmp(row[0], "NO")) {
			cnt_f2r_n++;
		}
	}
	mysql_free_result(res);

	total = cnt_f2r_y + cnt_f2r_n;
	if (total)
		stat->f2r = (float) ((float) cnt_f2r_y / (float) total);


}

void get_vpip(stat_t *stat) {
	char qbuf[QBUF];
	uint64_t gameid, prevgameid = 0;
	uint64_t cnt_total, cnt_act, cnt_pass, cnt_check, cnt_raise;
	uint64_t cnt_3bet, cnt_3bet_button, cnt_raise_button;
	int checked;
	float result;
	int button;
	
	snprintf(qbuf, QBUF, "select action, gameid, position from hands where playerid=%u AND state='PREFLOP' order by gameid, prio", ctx.selected_id);
	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}

	cnt_total = cnt_act = cnt_pass = cnt_check = cnt_raise = 0;
	cnt_3bet = cnt_3bet_button = cnt_raise_button = 0;
	res = mysql_use_result(conn);
	while ((row = mysql_fetch_row(res)) != NULL) {
		gameid = strtoll(row[1], NULL, 10);

		button = (!strcmp(row[2], "BUTTON")) ? 1 : 0;

		if (prevgameid == gameid && !checked) {
			continue;
		} else if (prevgameid && prevgameid != gameid && checked) {
			/* checks do not put money to a pot, even if they will see a flop */
			cnt_check++;
			cnt_total++;
		}

		checked = 0;
		if (!strcmp(row[0], "FOLD")) {
			cnt_pass++;
			cnt_total++;
		} else if (!strcmp(row[0], "CALL") || !strcmp(row[0], "RECALL")) {
			cnt_act++;
			cnt_total++;
		} else if (!strcmp(row[0], "RAISE")) {
			cnt_act++;
			cnt_total++;
			cnt_raise++;
			if (button)	
				cnt_raise_button++;
		} else if (!strcmp(row[0], "RERAISE")) {
			cnt_act++;
			cnt_total++;
			cnt_raise++;
			cnt_3bet++;
			if (button) {
				cnt_raise_button++;
				cnt_3bet_button++;
			}
		} else if (!strcmp(row[0], "CHECK")) {
			checked = 1;
		} else {
			printf("errror\n");
			exit(0);
		}


		prevgameid = gameid;
	}

	mysql_free_result(res);
	if (cnt_total != cnt_act + cnt_pass + cnt_check) {
		endwin();
		printf("Count error\n");
		exit(0);
	}

	result = (float)((float) cnt_act / (float) cnt_total);
	stat->vpip = result;

	cnt_act += cnt_check;
	result = (float)((float) cnt_act / (float) cnt_total);
	stat->vpip_check_as_act =  result;

	result = (float)((float) cnt_raise / (float) cnt_total);
	stat->pfr = result;

	result = (float)((float) cnt_raise_button / (float) cnt_total);
	//if (stat->pfr)
	stat->pfr_button = result;

	result = (float)((float) cnt_3bet / (float) cnt_total);
	stat->c3bet = result;

	result = (float)((float) cnt_3bet_button / (float) cnt_total);
	//if (stat->c3bet)
	//stat->c3bet_button = (result / stat->c3bet) * 100;
	stat->c3bet_button = result;
}

void common_tasks(char *state) {
	char qbuf[QBUF];
	char player[PLAYER_NAME_LEN];
	int i = 0;

	snprintf(qbuf, QBUF, "select avg(prob) as prob, count(prob) as count, count(*) as totcount,  action from hands where playerid=%u and state='%s' group by action order by action",
		ctx.selected_id, state);

	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}

	res = mysql_use_result(conn);
	while ((row = mysql_fetch_row(res)) != NULL) {
		if (!row[0]) {
			stat.common[i].prob = -1;
		} else {
			stat.common[i].prob = strtod(row[0], NULL);
		}
		
		stat.common[i].count = strtoul(row[1], NULL, 10);
		stat.common[i].total_count = strtoul(row[2], NULL, 10);
		strcpy(stat.common[i].action, row[3]);
		i++;
	}

	mysql_free_result(res);

	snprintf(qbuf, QBUF, "select name from players where id=%u", ctx.selected_id);
	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}
	
	res = mysql_use_result(conn);
	row = mysql_fetch_row(res);

	player[0] = 0;
	if (row != NULL) {
		strcpy(player, row[0]);
	}

	mysql_free_result(res);

	stat.common_count = i;
	get_vpip(&stat);
	get_stats(&stat, state);
	calc_af(&stat);

	show_common(player, state, &stat);


}

void inline show_item(char *name, float val) {
	if (!val)
		return;

	wprintw(w_table, " %s: ", name);
	wattron(w_table, A_BOLD);
	wprintw(w_table, "%.4f", val);
	wattroff(w_table, A_BOLD);

}

void show_common(char *player, char *state, stat_t *stat) {
	int i, count = stat->common_count;
	common_t *common = &stat->common[0];

	werase(w_table);

	wprintw(w_table, "Player ");
	wattron(w_table, A_BOLD);
	wprintw(w_table, "%s",player);
	wattroff(w_table, A_BOLD);
	wprintw(w_table, " on ");
	wattron(w_table, A_BOLD);
	wprintw(w_table, "%s %s",state, actions[ctx.action]);
	wattroff(w_table, A_BOLD);


	show_item("vpip", stat->vpip);
	show_item("(act", stat->vpip_check_as_act);

	wprintw(w_table, ") pfr/bt: ");
	wattron(w_table, A_BOLD);
	wprintw(w_table, "%.4f/%.4f", stat->pfr, stat->pfr_button);
	wattroff(w_table, A_BOLD);

	wprintw(w_table, " 3bet/bt: ");
	wattron(w_table, A_BOLD);
	wprintw(w_table, "%.4f/%.4f", stat->c3bet, stat->c3bet_button);
	wattroff(w_table, A_BOLD);

	show_item("af", stat->af);
	show_item("f2r", stat->f2r);
	show_item("chr", stat->chr);
	show_item("cb", stat->cb);
	show_item("db", stat->db);
	show_item("f2cb", stat->f2cb);

	show_item("no_folds", stat->no_folds);
	show_item("wins/nofolds", stat->wins);



	wprintw(w_table,"\n\n");



	for (i = 0; i < count; i++) {
		wprintw(w_table, "%.4f %4u-%u %4s\n", common->prob, common->count, common->total_count, common->action);
		common++;
	}


}

enum {
	ST_WAIT_SUIT,
	ST_WAIT_NUM
};

int parse_cards(char *buf, card_t *cards, uint8_t count) {
	int i, j, state = ST_WAIT_NUM;
	char c;

	for (j = 0, i = 0; i < count * 2; i++) {
		c = buf[i];

		switch (state) {
			case ST_WAIT_NUM:
				if (c > '9' || c < '1') {
					switch (c) {
						case 'A':
							cards[j].num = CARD_ACE;
							break;
						case 'K':
							cards[j].num = CARD_KING;
							break;
						case 'Q':
							cards[j].num = CARD_QUEEN;
							break;
						case 'J':
							cards[j].num = CARD_JACK;
							break;
						case 'T':
							cards[j].num = 10;
							break;
						default:
							return -1;
					}
				} else {
					cards[j].num = c - '0';
				}

				state = ST_WAIT_SUIT;
				break;

			case ST_WAIT_SUIT:
				switch (c) {
					case 's':
						cards[j].suit = SUIT_PIKI;
						break;
					case 'd':
						cards[j].suit = SUIT_BUBI;
						break;
					case 'c':
						cards[j].suit = SUIT_TREFI;
						break;
					case 'h':
						cards[j].suit = SUIT_CHERVI;
						break;
					default:
						return -1;

				}

				j++;
				state = ST_WAIT_NUM;
				break;
			default:
				return -1;
		}
			
	}

	return 0;
}

typedef struct pf_cnt_s {
	uint32_t s_cnt[78];
	uint32_t o_cnt[78];
	uint32_t p_cnt[13];
} pf_cnt_t;
pf_cnt_t cnt;

inline uint8_t mkidx(uint8_t c0, uint8_t c1) {
	uint8_t hc, lc, idx;
	int8_t prom;

	if (c0 > c1) {
		hc = c0;
		lc = c1;
	} else {
		hc = c1;
		lc = c0;
	}

	idx = 2 * 76 - 2 * lc - (hc * (hc - 5));
	idx /= 2;
	return idx;

}


void display_counters(void) {
	int i, j, c, col, idx;
	uint8_t hc, lc;
	int x, y, maxy;

	wprintw(w_table, "\n");

	maxy = getmaxy(w_table);

	for (i = 0; i < 13; i++) {
		wprint_card(w_table, CARD_ACE - i);
		wprint_card(w_table, CARD_ACE - i);

		if (cnt.p_cnt[i]) {
			wattron(w_table, A_BOLD);
			wprintw(w_table, " %u", cnt.p_cnt[i]);
			wattroff(w_table, A_BOLD);
		}
		wprintw(w_table, "\n", cnt.p_cnt[i]);
	}

	idx = 0;
	c = col = 0;
	wmove(w_table, 9, 8);
	for (i = 0; i < 12; i++) {
		hc = CARD_ACE - i;

		for (j = i + 1;  ; j++) {
			lc = CARD_ACE - j;

			y = getcury(w_table);
			x = 10 + 20 * col;

			wmove(w_table, y, x);

			wprint_card(w_table, hc);
			wprint_card(w_table, lc);

			wprintw(w_table, " s");
			if (cnt.s_cnt[idx]) {
				wattron(w_table, A_BOLD);
				wprintw(w_table, "%-3u ", cnt.s_cnt[idx]);
				wattroff(w_table, A_BOLD);
			}
			else
				wprintw(w_table, "    ");
			

			wprintw(w_table, " o");
			if (cnt.o_cnt[idx]) {
				wattron(w_table, A_BOLD);
				wprintw(w_table, "%-3u ", cnt.o_cnt[idx]);
				wattroff(w_table, A_BOLD);
			}
			else
				wprintw(w_table, "    ");

			wprintw(w_table, "\n");


	//		getyx(w_table, y, x);
			

			idx++;
			if (lc == 2) {
				break;
			}			

		c++;
			if (c == maxy - 15) {
				c = 0;
				col++;

				wmove(w_table, 9, x);
	//			wprint_card(w_table, hc);
	//			wprint_card(w_table, lc);

			}

				
		}

	}
	

	//wprintw(w_table, "xxxxxxxxxxxxxxxxxx\n");


	wrefresh(w_table);
}

char *combs_text[] = {
	"none",
	"overpair",
	"top pair",
	"middle pair",
	"bottom pair",
	"double pair",
	"triplet",
	"straight",
	"flush",
	"full house",
	"care",
	"str. flush",
	"royal flush"
};

void display_main(uint64_t total) {
	uint64_t i, stotal;
	mcnt_t *m;
	

	wprintw(w_table, "\n");

	wattron(w_table, A_BOLD);
	wprintw(w_table, "combination\t\ttotal\t\tno_draw\t\tfdraw\t\tsdraw\t\tgutshot\t\tfd+sd\t\tfd+gs\n");
	wattroff(w_table, A_BOLD);
	for (i = 0; i < MAX_COMB2; i++) {
		m = &mcnt[i];

		stotal = m->cnt + m->fdraw + m->sdraw + m->gs + m->gs_fdraw + m->sdraw_fdraw;
		wprintw(w_table, "%10s\t\t%6llu\t\t%6llu\t\t%6llu\t\t%6llu\t\t%6llu\t\t%6llu\t\t%6llu\n", 
			combs_text[i], stotal, m->cnt, m->fdraw, m->sdraw, m->gs, m->sdraw_fdraw, m->gs_fdraw);
	}


	wrefresh(w_table);
}


void process_preflop(void) {
	char qbuf[QBUF];
	card_t cards[2];
	uint8_t idx;
	

	common_tasks("PREFLOP");
	snprintf(qbuf, QBUF, "select cards from hands where playerid=%u and state='PREFLOP' and prob IS NOT NULL and action='%s' order by action",
		ctx.selected_id, actions[ctx.action]);

	memset(&cnt, 0, sizeof(pf_cnt_t));

//	endwin();
//	printf("q=%s\n",qbuf);
//	exit(0);

	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}

	res = mysql_use_result(conn);
	while ((row = mysql_fetch_row(res)) != NULL) {
		if (parse_cards(row[0], cards, 2) < 0) {
			endwin();
			fprintf(stderr, "Parse error %s\n",row[0]);
			exit(1);
		}

		if (cards[0].num == cards[1].num) {
			cnt.p_cnt[CARD_ACE - cards[0].num]++;
			continue;
		}

		idx = mkidx(cards[0].num, cards[1].num);
		if (cards[0].suit == cards[1].suit) {
			cnt.s_cnt[idx]++;
		} else {
			cnt.o_cnt[idx]++;
		}

	}

	mysql_free_result(res);
	display_counters();
}

void main_task(char *state) {
	char qbuf[QBUF];
	uint8_t type, idx;
	uint32_t draw;
	uint64_t total;

	memset(mcnt, 0, sizeof(mcnt_t) * MAX_COMB2);
	snprintf(qbuf, QBUF, "select combination,prob,draw from hands where playerid=%u and state='%s' and action='%s'",
			ctx.selected_id, state, actions[ctx.action]);

	if (mysql_query(conn, qbuf)) {
		endwin();
		fprintf(stderr, "Query %s failed: %s\n", qbuf, mysql_error(conn));
		exit(1);
	}

	total = 0;
	res = mysql_use_result(conn);
	while ((row = mysql_fetch_row(res)) != NULL) {
		if (!row[0])
			continue;

		type = strtoll(row[0], NULL, 10);
		draw = strtoll(row[2], NULL, 10);

		if (type == COMB_NONE) {
			idx = type;
		} else if (type == COMB_SINGLE_PAIR) {
			if (draw & RET_OVER_PAIR) {
				idx = 1;
			} else if (draw & RET_TOP_PAIR) {
				idx = 2;
			} else if (draw & RET_MIDDLE_PAIR) {
				idx = 3;
			} else if (draw & RET_BOTTOM_PAIR) {
				idx = 4;	
			} else {
				endwin();
				fprintf(stderr, "Something goes wrong\n");
				exit(1);
			}
		} else {
			idx = type + 3;
		}

		if (idx > MAX_COMB2) {
			endwin();
			fprintf(stderr, "Something goes wrong\n");
			exit(1);
		}

		if (draw & RET_FLUSH_DRAW) {
			if (draw & RET_STRAIGHT_DRAW) {
				mcnt[idx].sdraw_fdraw++;
			} else if (draw & RET_GUT_SHOT) {
				mcnt[idx].gs_fdraw++;
			} else {
				mcnt[idx].fdraw++;
			}
		} else if (draw & RET_STRAIGHT_DRAW) {
			mcnt[idx].sdraw++;
		} else if (draw & RET_GUT_SHOT) {
			mcnt[idx].gs++;
		} else
			mcnt[idx].cnt++;
		
		total++;
	}

	display_main(total);
	mysql_free_result(res);
}

void game_tasks(char *state) {
	get_oth_stats(&stat, state);
	get_wsd(&stat, state);
	common_tasks(state);
	main_task(state);
}

void process_flop(void) {
	game_tasks("FLOP");
	wrefresh(w_table);
}

void process_turn(void) {
	game_tasks("TURN");
	wrefresh(w_table);
}

void process_river(void) {
	game_tasks("RIVER");
	wrefresh(w_table);
}


void process_finish(void) {
	init_ctx();
}

void process(void) {
	int state;
	float pr;

	memset(&stat, 0, sizeof(stat_t));
	switch (ctx.state) {
		case ST_PREFLOP:
			process_preflop();
			break;
		case  ST_FLOP:
			process_flop();
			break;
		case ST_TURN:
			process_turn();
			break;
		case ST_RIVER:
			process_river();
			break;
		case ST_FINISH:
			process_finish();
			break;
	}
	
	ctx.mode = MODE_SHOW;
	//ctx.state = 0;
	ctx.idx = 0;
	memset(&ctx, 0, PLAYER_NAME_LEN);
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

	touchwin(w_main);
	wrefresh(w_table);
	wrefresh(w_main);

}

uint8_t get_state(void) {
	FILE *fs;
	uint8_t state;

	fs = fopen(ST_FILE, "r");
	if (!fs)
		return ST_PREFLOP;

	if (!fscanf(fs, "%hhu", &state))
		return 0;
	fclose(fs);
	if (state > ST_RIVER)
		return ST_PREFLOP;

	return state;
}

void init_ctx(void) {
	memset(&ctx, 0, PLAYER_NAME_LEN);
	ctx.state = get_state();
	ctx.idx = 0;
	ctx.changed = ctx.bs = 0;
	ctx.mode = MODE_INPUT;
	ctx.selected_id = 0;
	ctx.action = 0;
}

uint32_t inline get_curr_idx(void) {
	int i;

	for (i = 0; i < NPLAYERS; i++) {
		if (ctx.players[i].id == ctx.selected_id) 
			return i;
	}
	
	return 0;
}


int main(void) {
	uint32_t ch;
	card_t card;
	int bs, idx;

	init_system();
	init_ctx();

	print_bottom();
	while (1) {
		//print_main();
		print_bottom();

		ch = getch();
	

		ctx.need_db_req = 0;
		ctx.stop = ctx.bs = ctx.changed = 0;
		switch (ch) {
			case '>':
				ctx.stop = 1;
				if (ctx.state < ST_RIVER)
					ctx.state++;

				process();
				break;
			case '<':
				ctx.stop = 1;
				if (ctx.state > 0)
					ctx.state--;
				
				process();
				break;
			
			case KEY_RIGHT:
				ctx.stop = 1;
				if (ctx.action < MAX_ACTIONS - 1)
					ctx.action++;

				process();
				break;
			case KEY_LEFT:
				ctx.stop = 1;
				if (ctx.action > 0)
					ctx.action--;

				process();
				break;
			case KEY_UP:
				idx = get_curr_idx();	
				ctx.stop = 1;			

				if (idx <= 0 || !ctx.players[0].id) 
					break;

				ctx.selected_id = ctx.players[idx - 1].id;
				break;
			case KEY_DOWN:	
				idx = get_curr_idx();	
				ctx.stop = 1;			

				if (idx >= NPLAYERS - 1 || !ctx.players[0].id) 
					break;

				if (!ctx.players[idx + 1].id)
					break;

				ctx.selected_id = ctx.players[idx + 1].id;
				break;
			case KEY_BACKSPACE:
				if (!ctx.idx)
					continue;
				ctx.idx--;
				ctx.name[ctx.idx] = 0;
				ctx.bs = 1;
				ctx.selected_id = 0;
				ctx.need_db_req = 1;
				break;		
			case 10:
				process();
				ctx.stop = ctx.changed = 1;
				break;
			default:
				ctx.need_db_req = 1;
				ctx.selected_id = 0;
				break;
		}

		if (ctx.idx < PLAYER_NAME_LEN && !ctx.bs && !ctx.stop) {
			ctx.name[ctx.idx++] = ch;
		}

		if (ctx.mode == MODE_INPUT)
			show_player_list();
	

	}
}
