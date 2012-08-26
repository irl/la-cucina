/**
 * Copyright (C) 2012 Iain R. Learmonth <irl@sdf.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "str_replace.h"

#define MAX_LEVEL_ROWS 25
#define MAX_LEVEL_COLS 80

typedef struct order Order;

struct order
{
	char *name;
	time_t time;
	int pizza;
	Order *next;
};

int currency = 0;
int pepperoni = 3;
int cheese = 3;
int tomato = 3;
int garlic = 3;
int dough = 4;

Order *orders;
int ordercount = 0;
int mordercount = 0;
int fordercount = 0;
int dordercount = 0;

int isneworder;
Order *neworder = NULL;

int x = 13;
int y = 10;

int holding = 0;

int oven[4] = { 0, 0, 0, 0 };
int otim[4];

char **level;

char status[80];

WINDOW *front;

int row, col;

char *
pizza_name(int pizza)
{
	pizza ^= 32;

	switch ( pizza )
	{
		case 3:
			return "Garlic Bread";
		case 11:
			return "Cheesy Garlic Bread";
		case 13:
			return "Cheese Pizza";
		case 29:
			return "Pepperoni Pizza";
		default:
			return "WTF";
	}
}

int
pizza_price(int pizza)
{
	pizza ^= 32;

	switch ( pizza )
	{
		case 3:
			return 3;
		case 11:
			return 4;
		case 13:
			return 5;
		case 29:
			return 6;
		default:
			return 200;
	}
}


char *
random_name()
{
    FILE *f;
    int nLines = 0;
    char line[1024];
    int randLine;
    int i;
	char *name;

	name = malloc(1024);

    srand(time(0));
    f = fopen("names.txt", "r");

	/* 1st pass - count the lines. */
    while(!feof(f))
    {
        fgets(line, 1024, f);
        nLines++;
    }

    randLine = rand() % nLines;

	/* 2nd pass - find the line we want. */
    fseek(f, 0, SEEK_SET);
    for(i = 0; !feof(f) && i <= randLine; i++)
        fgets(line, 1024, f);

	line[strlen(line) - 1] = '\0';

	if ( rand() % 2 == 0 )
	{
	    strcpy(name, "Mr. ");
	}
	else
	{
	    strcpy(name, "Mrs. ");
	}
		
	strcat(name, line);

	return name;
}

WINDOW *
create_newwin(int height, int width)
{
	WINDOW *local_win;
	int i, j;

	i = (col - width) / 2;
	j = (row - height) / 2;

	local_win = newwin(height, width, j, i);
	box(local_win, 0 , 0);
	wrefresh(local_win);

	return local_win;
}

void destroy_win(WINDOW *local_win)
{	
	/* box(local_win, ' ', ' '); : This won't produce the desired
	 * result of erasing the window. It will leave it's four corners 
	 * and so an ugly remnant of window. 
	 */
	wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	wrefresh(local_win);
	delwin(local_win);
}

void
draw_screen()
{
	int i;

	char* h1 = malloc(50);
	char* h2 = malloc(50);
	char* h3 = malloc(50);
	char* h4 = malloc(50);
	char* h5 = malloc(50);
	char* h6 = malloc(50);

	char* cur = malloc(50);

	if ( ! h1 || ! h2 || ! h3 || ! h4 || ! h5 || ! h6 )
	{
		endwin();
		printf("Ran out of memory!");
		exit(255);
	}

	strcpy(h1, "");
	strcpy(h2, "");
	strcpy(h3, "");
	strcpy(h4, "");
	strcpy(h5, "");
	strcpy(h6, "");

	erase();

	getmaxyx(stdscr, row, col);

	if ( row < MAX_LEVEL_ROWS || col < MAX_LEVEL_COLS )
	{
		attroff(COLOR_PAIR(2));
		attroff(A_BOLD);
		printw("Terminal must have at least %d rows and %d columns.", MAX_LEVEL_ROWS, MAX_LEVEL_COLS);
		refresh();
		return;
	}

	init_pair(1, COLOR_YELLOW, COLOR_RED);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	init_pair(3, COLOR_YELLOW, COLOR_BLACK);

	attron(A_BOLD);
	attron(COLOR_PAIR(1));

	mvprintw(0, 0, "");

	for ( i = 0 ; i < col ; ++i )
	{
		printw(" ");
	}

	mvprintw(row - 1, 0, "");

	for ( i = 0 ; i < col ; ++i )
	{
		printw(" ");
	}
	
	i = ( col - strlen("La Cucina") ) / 2;

	mvprintw(0, i, "La Cucina");
	mvprintw(row - 1, 0, "%s", status);

	sprintf(cur, "Budget: %d", currency);

	mvprintw(row - 1, col - 1 - strlen(cur), cur);

	free(cur);

	attron(COLOR_PAIR(2));

	if ( holding & 1 )
	{
		strcpy(h2, "PIZZA BASE");
	}

	if ( holding & 2 )
	{
		strcpy(h3, "GARLIC SAUCE");
	}

	if ( holding & 4 )
	{
		strcpy(h3, "TOMATO SAUCE");
	}

	if ( holding & 8 )
	{
		strcpy(h4, "CHEESE");
	}

	if ( holding & 16 )
	{
		strcpy(h5, "PEPPERONI");
	}

	if ( holding & 32 )
	{
		strcpy(h1, "COOKED");
	}

	for ( i = 2 ; i < MAX_LEVEL_ROWS ; ++i )
	{
		char* buf = malloc(MAX_LEVEL_COLS + 1);
		char* os = malloc(10);
		int o;

		strcpy(buf, (char *) level[i]);

		buf = str_replace(buf, "<H1>", h1);
		buf = str_replace(buf, "<H2>", h2);
		buf = str_replace(buf, "<H3>", h3);
		buf = str_replace(buf, "<H4>", h4);
		buf = str_replace(buf, "<H5>", h5);
		buf = str_replace(buf, "<H6>", h6);

		sprintf(os, "%d", ordercount);

		buf = str_replace(buf, "<OO>", os);

		sprintf(os, "%d", mordercount);

		buf = str_replace(buf, "<MM>", os);

		sprintf(os, "%d", fordercount);

		buf = str_replace(buf, "<FF>", os);

		sprintf(os, "%d", dordercount);

		buf = str_replace(buf, "<DD>", os);

		for ( o = 0 ; o < 4 ; ++o )
		{
			if ( oven[o] != 0 && ((int) time(NULL)) % 2 == 0 )
			{
				sprintf(os, "%d|", o);
				if ( oven[o] & 32 )
				{
					buf = str_replace(buf, os, "P|");
				}
				else
				{
					buf = str_replace(buf, os, "C|");
				}
			}
		}

		if ( isneworder && ((int) time(NULL)) % 2 == 0 )
		{
			buf = str_replace(buf, "T|", "#|");
		}

		mvprintw(i, 0, buf);

		free(os);
		free(buf);
	}

	if ( ( ((int) time(NULL)) % 2 == 0 ) && holding != 0 )
	{
		mvprintw(y, x, "P");
	}
	else
	{
		attron(COLOR_PAIR(3));
		mvprintw(y, x, "@");
		attron(COLOR_PAIR(2));
	}

	refresh();

	free(h1);
	free(h2);
	free(h3);
	free(h4);
	free(h5);
	free(h6);
}

void
make_move(int i, int j)
{
	int valid_move = 1;

	if ( level[y+j][x+i] != ' ' )
	{
		valid_move = 0;
	}

	if ( ! valid_move )
	{
		strcpy(status, "You cannot walk through walls or pizza mess.");
	} else {
		x += i;
		y += j;
	}
}

void
pickup()
{
	int cy, cx, i;

	for ( cy = -1 ; cy < 2 ; ++cy )
	{
		for ( cx = -1 ; cx < 2 ; ++cx )
		{
			int o = 0;

			if ( ( ! ( cy == 0 || cx == 0 ) ) ||
				( cy == 0 && cx == 0 ) )
			{
				continue;
			}
			switch ( level[y+cy][x+cx] )
			{
				case 'd':
					if ( holding == 0 )
					{
						if ( dough == 0 )
						{
							strcpy(status, "You don't have enough dough.");
							goto picked;
						}
						holding = 1;
						--dough;
						goto picked;
					}
					break;
				case 'g':
					if ( holding == 1 )
					{
						if ( garlic == 0 )
						{
							strcpy(status, "You don't have enough garlic sauce.");
							goto picked;
						}
						holding |= 2;
						--garlic;
						goto picked;
					}
					break;
				case 't':
					if ( holding == 1 )
					{
						if ( tomato == 0 )
						{
							strcpy(status, "You don't have enough tomato sauce.");
							goto picked;
						}
						holding |= 4;
						--tomato;
						goto picked;
					}
					break;
				case 'c':
					if ( holding == 3 || holding == 5 )
					{
						if ( cheese == 0 )
						{
							strcpy(status, "You don't have enough cheese.");
							goto picked;
						}
						holding |= 8;
						--cheese;
						goto picked;
					}
					break;
				case 'p':
					if ( ( ( holding & 8 ) != 0 ) && ( ( holding & 2 ) == 0 ) )
					{
						if ( pepperoni == 0 )
						{
							strcpy(status, "You don't have enough pepperoni.");
							goto picked;
						}
						holding |= 16;
						--pepperoni;
						goto picked;
					}
					break;
				case '1':
					o = 1;
				case '2':
					if ( o == 0 ) o = 2;
				case '3':
					if ( o == 0 ) o = 3;
					if ( oven[o] != 0 && ((int) time(NULL)) > otim[o] + 20 && holding == 0 )
					{
						holding = oven[o];
						oven[o] = 0;
						goto picked;
					}
					break;
				case 'T':
					if ( ordercount > 9 )
					{
						front = create_newwin(10, 60);
						mvwprintw(front, 2, 5, "You're too busy to answer the phone right now.");
						wrefresh(front);
						while ( getch() != 'o' );
					}
					if ( isneworder )
					{
						Order* lastorder;

						front = create_newwin(10, 60);
						mvwprintw(front, 2, 5, "%s ordered a %s.", neworder->name, pizza_name(neworder->pizza));
						wrefresh(front);
						while ( getch() != 'o' );
						if ( ordercount == 0 )
						{
							lastorder = neworder;
							orders = neworder;
						}
						else
						{
							lastorder = orders;
							for ( i = 1 ; i < ordercount ; ++i )
							{
								lastorder = lastorder->next;
							}
						}
						lastorder->next = neworder;
						neworder->next = orders;
						++ordercount;

						isneworder = 0;
					}
					goto picked;
					break;
			}
		}
	}
	strcpy(status, "Your hands are full or there's nothing there.");
	picked: return;
}

void
notepad()
{
	if ( ordercount == 0 )
	{
		strcpy(status, "No outstanding orders. Relax!");
		return;
	}

	front = create_newwin(ordercount + 2, 60);

	int i = 0;
	Order *order;

	for ( order = orders ; i < ordercount ; order = order->next )
	{
		mvwprintw(front, i + 1, 5, "%s ordered a %s.", order->name, pizza_name(order->pizza));
		++i;
	}

	wrefresh(front);

	while ( getch() != 'o' );

}

int
deliver()
{
	front = create_newwin(ordercount + 2, 60);
	int cust;
	int i = 0;
	Order *order;

	for ( order = orders ; i < ordercount ; order = order->next )
	{
		mvwprintw(front, i + 1, 5, "%d. %s ordered a %s.", i + 1, order->name, pizza_name(order->pizza));
		++i;
	}

	wrefresh(front);

	while ( ! ( ( cust = getch() ) > '0' && cust <= ( '0' + ordercount ) ) );

	char custs[2] = { cust, 0 };

	int custi = atoi(custs);

	i = 1;

	for ( order = orders ; i < custi ; order = order->next ) ++i;

	if ( order->pizza == holding )
	{
		mvprintw(0,0,"%d %d", order->pizza, holding);
		++dordercount;
		currency += pizza_price(order->pizza);
	}
	else
	{
		++fordercount;
	}

	if ( ordercount == 1 )
	{
		free(orders);

		ordercount = 0;
	}
	else if ( custi == 1 )
	{
		i = 0;

		Order *old;

		for ( order = orders ; i < ordercount ; order = order->next ) ++i;

		order->next = orders->next;

		old = orders;

		orders = orders->next;

		free(old);

		--ordercount;
	}
	else
	{
		i = 0;

		Order *old;

		for ( order = orders ; i < custi - 2 ; order = order->next ) ++i;

		old = order->next;

		order->next = order->next->next;

		free(old);

		--ordercount;
	}
}

void
drop()
{
	int cy, cx;

	if ( holding == 0 )
	{
		return;
	}

	for ( cy = -1 ; cy < 2 ; ++cy )
	{
		for ( cx = -1 ; cx < 2 ; ++cx )
		{
			if ( ( ! ( cy == 0 || cx == 0 ) ) ||
				( cy == 0 && cx == 0 ) )
			{
				continue;
			}
			if ( ( level[y+cy][x+cx] == '1' ||
				level[y+cy][x+cx] == '2' ||
				level[y+cy][x+cx] == '3' ) && ( holding & 32 != 0 ) )
			{
				char oc[2] = { level[y+cy][x+cx], 0 };
				oven[atoi(oc)] = holding;
				otim[atoi(oc)] = time(NULL);
				holding = 0;
				goto dropped;
			}
		}
	}
	for ( cy = -1 ; cy < 2 ; ++cy )
	{
		for ( cx = -1 ; cx < 2 ; ++cx )
		{
			if ( ( ! ( cy == 0 || cx == 0 ) ) ||
				( cy == 0 && cx == 0 ) )
			{
				continue;
			}
			if ( level[y+cy][x+cx] == 'D' && ordercount != 0 )
			{
				int cust = deliver();
				holding = 0;
				goto dropped;
			}
		}
	}
	for ( cy = -1 ; cy < 2 ; ++cy )
	{
		for ( cx = -1 ; cx < 2 ; ++cx )
		{
			if ( ( ! ( cy == 0 || cx == 0 ) ) ||
				( cy == 0 && cx == 0 ) )
			{
				continue;
			}
			if ( level[y+cy][x+cx] == ' ' )
			{
				level[y+cy][x+cx] = '!';
				holding = 0;
				goto dropped;
			}
		}
	}
	strcpy(status, "It's not possible to drop this item right now.");
	dropped: return;
}

void
shop()
{
	char ch;
	char* cur = malloc(50);

	front = create_newwin(7, 70);

	attron(COLOR_PAIR(1));

	while ( ( ch = getch() ) != 'o' )
	{
		sprintf(cur, " Budget: %d", currency);
	
		mvprintw(row - 1, col - 1 - strlen(cur), cur);

		wrefresh(stdscr);
	
		mvwprintw(front, 2, 2, "Dough: %d\tGarlic Sauce: %d  \tTomato Sauce: %d", dough, garlic, tomato);
	
		mvwprintw(front, 4, 2, "Cheese: %d\tPepperoni: %d", cheese, pepperoni);
	
		wrefresh(front);

		if ( currency == 0 )
		{
			continue;
		}

		switch ( ch )
		{
			case 'd':
				++dough;
				--currency;
				break;
			case 'g':
				++garlic;
				--currency;
				break;
			case 't':
				++tomato;
				--currency;
				break;
			case 'c':
				++cheese;
				--currency;
				break;
			case 'p':
				++pepperoni;
				--currency;
				break;
		}
	}

	free(cur);

	destroy_win(front);
}

void
handle_events()
{
	char c = getch();

	if ( c > 0 )
	{
		if ( isneworder )
		{
			strcpy(status, "Ring Ring, Ring Ring....");
		}
		else
		{
			strcpy(status, "Play the game.");
		}
	}

	switch ( c )
	{
		case 'q':
			endwin();
			exit(0);
			break;
		case 'h':
			make_move(-1, 0);
			break;
		case 'j':
			make_move(0, 1);
			break;
		case 'k':
			make_move(0, -1);
			break;
		case 'l':
			make_move(1, 0);
			break;
		case 'y':
			pickup();
			break;
		case 'p':
			drop();
			break;
		case 'b':
			notepad();
			break;
		case 'i':
			shop();
			break;
	}
}

void
load_level_data()
{
	int y;

	FILE *fh = fopen("level.txt", "r");
	
	level = (char **) malloc(MAX_LEVEL_ROWS);

	for ( y = 0 ; y < MAX_LEVEL_ROWS ; ++y )
	{
		char *row = (char *) malloc(MAX_LEVEL_COLS + 1);

		fgets(row, MAX_LEVEL_COLS + 1, fh);

		level[y] = row;
	}

}

int
main(int argc, char** argv)
{
	int o;
	int interval;

	/*
     * ncurses initialisation
	 */

	initscr();				/* Start ncurses mode */
	raw();					/* Line buffering disabled */
	keypad(stdscr, TRUE);	/* We get F1, F2 etc.. */
	noecho();				/* Don't echo() while we do getch */
	timeout(10);			/* Don't block on getch */
	curs_set(0);			/* Disable the cursor */
	start_color();			/* colours make it look less old */

	load_level_data();

	for ( ;; )
	{
		draw_screen();
		handle_events();
		for ( o = 1 ; o < 4 ; ++o )
		{
			if ( oven[o] != 0 && ((int) time(NULL)) > otim[o] + 20 )
			{
				oven[o] |= 32;
			}
		}
		if ( ( (int) time(NULL) % 5 ) == 0 && isneworder == 0 )
		{
			isneworder = 1;
			neworder = (Order *) malloc(sizeof(Order));
			neworder->name = random_name();
			neworder->pizza  = 0;
			neworder->pizza |= 1 | 32;
			if ( rand() % 2 == 0 )
			{
				neworder->pizza |= 2;
				if ( rand() % 2 == 0 )
				{
					neworder->pizza |= 8;
				}
			}
			else
			{
				neworder->pizza |= 4;
				neworder->pizza |= 8;
				if ( rand() % 2 == 0 )
				{
					neworder->pizza |= 16;
				}
			}
		}
		else if ( ( ( (int) time(NULL) + 4 ) % 5 ) == 0 && isneworder == 1 )
		{
			free(neworder);
			isneworder = 0;
			++mordercount;
		}
		usleep(100);
	}
}
