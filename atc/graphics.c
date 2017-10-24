/*	$NetBSD: graphics.c,v 1.10 2003/08/07 09:36:54 agc Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ed James.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */

/*
 * Color support and plane direction vectors added by Oliver B <oliver.77.b@gmail.com>
 * Coded and uploaded on 24/Oct/2017
 * No has_colors(); check performed: use this program on color terminals...
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)graphics.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: graphics.c,v 1.10 2003/08/07 09:36:54 agc Exp $");
#endif
#endif /* not lint */

#include "include.h"

#define C_TOPBOTTOM		'-'
#define C_LEFTRIGHT		'|'
#define C_AIRPORT		'='
#define C_LINE			'+'
#define C_BACKROUND		'.'
#define C_BEACON		'*'
#define C_CREDIT		'*'

/* COLOR_PAIR ID for the screen elements.
 * Frame, "Grass" is Radar background, Flight Routes
 * Navigation points are blue background: Exits, Airports, beacons
 * Four plane colors: "PR" is for prop, "JT" is for Jet, "_1" is for marked, "_0" is for unmarked.
 * Fuel warning color and list header for the right side pane, and then the direction vector colors
 */

#define COL_FRAME 1
#define COL_GRASS 2
#define COL_ROUTE 3
#define COL_EXIT  4
#define COL_AIRPT 5
#define COL_BEACN 6
#define COL_PL_PR_1 7
#define COL_PL_JT_1 8
#define COL_PL_PR_0 9
#define COL_PL_JT_0 10
#define COL_FUEL  11
#define COL_HEAD  12
#define COL_VECTR_PR 13
#define COL_VECTR_JT 14

WINDOW	*radar, *cleanradar, *credit, *input, *planes;

int
getAChar()
{
	int c;

	errno = 0;
	while ((c = getchar()) == EOF && errno == EINTR) {
		errno = 0;
		clearerr(stdin);
	}
	return(c);
}

void
erase_all()
{
	PLANE	*pp;
	int dx,dy;

// In case of erasing a plane from the radar a bigger area needs to be cleaned because of the speed vector...
	for (pp = air.head; pp != NULL; pp = pp->next) {
		for (dx=-1; dx<2; dx++) {
			for (dy=-1; dy<2; dy++) {
				if ((pp->xpos+dx >= 0) && (pp->xpos+dx < sp->width) && (pp->ypos+dy >= 0) && (pp->ypos+dy < sp->height)) {
					wmove(cleanradar, pp->ypos+dy, (pp->xpos +dx)*2);
					wmove(radar, pp->ypos+dy, (pp->xpos+dx)*2);
					waddch(radar, winch(cleanradar));
					wmove(cleanradar, pp->ypos+dy, (pp->xpos +dx)*2+1);
					wmove(radar, pp->ypos+dy, (pp->xpos+dx)*2+1);
					waddch(radar, winch(cleanradar));
				}
			}
		}
	}
}

void
draw_all()
{
	PLANE	*pp;
  char myname[5];

	for (pp = air.head; pp != NULL; pp = pp->next) {
// Decide plane color and print name first
		sprintf(myname,"%c",name(pp));
		if (pp->status == S_MARKED) {
			if ((myname[0] > 64) && (myname[0] < 91)) {
				wattron(radar, COLOR_PAIR(COL_PL_PR_1)|A_BOLD);
			} else {
				wattron(radar, COLOR_PAIR(COL_PL_JT_1)|A_BOLD);
			}
		} else {
			if ((myname[0] > 64) && (myname[0] < 91)) {
				wattron(radar, COLOR_PAIR(COL_PL_PR_0));
			} else {
				wattron(radar, COLOR_PAIR(COL_PL_JT_0));
			}
		}
		wmove(radar, pp->ypos, pp->xpos * 2);
		waddch(radar, name(pp));
		waddch(radar, '0' + pp->altitude);
// Decide color for the direction vector
		if ((myname[0] > 64) && (myname[0] < 91)) {
			wattron(radar, COLOR_PAIR(COL_VECTR_PR));
		} else {
			wattron(radar, COLOR_PAIR(COL_VECTR_JT));
		}
// Based on plane direction go to appropriate position and print the appropriate char as a speed vector
		switch (pp->dir) {
			case 0: wmove(radar, pp->ypos-1, pp->xpos*2  ); waddch(radar, '|'); break;
			case 1: wmove(radar, pp->ypos-1, pp->xpos*2+2); waddch(radar, '/'); break;
			case 2:                     waddch(radar, '-'); waddch(radar, '-'); break;
			case 3: wmove(radar, pp->ypos+1, pp->xpos*2+2); waddch(radar, '\\'); break;
			case 4: wmove(radar, pp->ypos+1, pp->xpos*2  ); waddch(radar, '|'); break;
			case 5: wmove(radar, pp->ypos+1, pp->xpos*2-1); waddch(radar, '/'); break;
			case 6: wmove(radar, pp->ypos,   pp->xpos*2-2); waddch(radar, '-'); waddch(radar, '-'); break;
			case 7: wmove(radar, pp->ypos-1, pp->xpos*2-1); waddch(radar, '\\'); break;
		}

		wattrset(radar, 0);
	}
	wrefresh(radar);
	planewin();
	wrefresh(input);		/* return cursor */
	fflush(stdout);
}

void
init_gr()
{
	static char	buffer[BUFSIZ];

	initscr();
	start_color();
/* Here You can adjust any color for the screen elements.
 * At the moment colors are hardcoded,
 * but if someone wants to have own color set,
 * we can read the color pairs from an .rc file.
 */
	init_pair(COL_FRAME,    COLOR_BLUE,    COLOR_BLACK);
	init_pair(COL_GRASS,    COLOR_GREEN,   COLOR_BLACK);
	init_pair(COL_ROUTE,    COLOR_BLUE,    COLOR_BLACK);
	init_pair(COL_EXIT,     COLOR_CYAN,    COLOR_BLUE);
	init_pair(COL_AIRPT,    COLOR_GREEN,   COLOR_BLUE);
	init_pair(COL_BEACN,    COLOR_MAGENTA, COLOR_BLUE);
	init_pair(COL_PL_PR_1,  COLOR_YELLOW,  COLOR_YELLOW);
	init_pair(COL_PL_JT_1,  COLOR_RED,     COLOR_RED);
	init_pair(COL_PL_PR_0,  COLOR_YELLOW,  COLOR_BLACK);
	init_pair(COL_PL_JT_0,  COLOR_RED,     COLOR_BLACK);
	init_pair(COL_FUEL,     COLOR_MAGENTA, COLOR_BLACK);
	init_pair(COL_HEAD,     COLOR_CYAN,    COLOR_CYAN);
	init_pair(COL_VECTR_PR, COLOR_YELLOW,  COLOR_BLACK);
	init_pair(COL_VECTR_JT, COLOR_RED,     COLOR_BLACK);

	setbuf(stdout, buffer);
	input = newwin(INPUT_LINES, COLS - PLANE_COLS, LINES - INPUT_LINES, 0);
	credit = newwin(INPUT_LINES, PLANE_COLS, LINES - INPUT_LINES,
		COLS - PLANE_COLS);
	planes = newwin(LINES - INPUT_LINES, PLANE_COLS, 0, COLS - PLANE_COLS);
}

void
setup_screen(scp)
	const C_SCREEN	*scp;
{
	int	i, j;
	char	str[3];
	const char *airstr;

	str[2] = '\0';

	if (radar != NULL)
		delwin(radar);
	radar = newwin(scp->height, scp->width * 2, 0, 0);

	if (cleanradar != NULL)
		delwin(cleanradar);
	cleanradar = newwin(scp->height, scp->width * 2, 0, 0);

	/* minus one here to prevent a scroll */
	for (i = 0; i < PLANE_COLS - 1; i++) {
		wmove(credit, 0, i);
		waddch(credit, C_CREDIT);
		wmove(credit, INPUT_LINES - 1, i);
		waddch(credit, C_CREDIT);
	}
	wmove(credit, INPUT_LINES / 2, 1);
	waddstr(credit, AUTHOR_STR);
	wattron(radar, COLOR_PAIR(COL_GRASS));
	for (i = 1; i < scp->height - 1; i++) {
		for (j = 1; j < scp->width - 1; j++) {
			wmove(radar, i, j * 2);
			waddch(radar, C_BACKROUND);
		}
	}

	/*
	 * Draw the lines first, since people like to draw lines
	 * through beacons and exit points.
	 */
	wattron(radar, COLOR_PAIR(COL_ROUTE));
	str[0] = C_LINE;
	for (i = 0; i < scp->num_lines; i++) {
		str[1] = ' ';
		draw_line(radar, scp->line[i].p1.x, scp->line[i].p1.y,
			scp->line[i].p2.x, scp->line[i].p2.y, str);
	}

	wattron(radar, COLOR_PAIR(COL_FRAME));
	str[0] = C_TOPBOTTOM;
	str[1] = C_TOPBOTTOM;
	wmove(radar, 0, 0);
	for (i = 0; i < scp->width - 1; i++)
		waddstr(radar, str);
	waddch(radar, C_TOPBOTTOM);

	str[0] = C_TOPBOTTOM;
	str[1] = C_TOPBOTTOM;
	wmove(radar, scp->height - 1, 0);
	for (i = 0; i < scp->width - 1; i++)
		waddstr(radar, str);
	waddch(radar, C_TOPBOTTOM);

	for (i = 1; i < scp->height - 1; i++) {
		wmove(radar, i, 0);
		waddch(radar, C_LEFTRIGHT);
		wmove(radar, i, (scp->width - 1) * 2);
		waddch(radar, C_LEFTRIGHT);
	}

	wattron(radar, COLOR_PAIR(COL_BEACN)|A_BOLD);
	str[0] = C_BEACON;
	for (i = 0; i < scp->num_beacons; i++) {
		str[1] = '0' + i;
		wmove(radar, scp->beacon[i].y, scp->beacon[i].x * 2);
		waddstr(radar, str);
	}

	wattron(radar, COLOR_PAIR(COL_EXIT)|A_BOLD);
	for (i = 0; i < scp->num_exits; i++) {
		wmove(radar, scp->exit[i].y, scp->exit[i].x * 2);
		waddch(radar, '0' + i);
	}

	wattron(radar, COLOR_PAIR(COL_AIRPT));
	airstr = "^?>?v?<?";
	for (i = 0; i < scp->num_airports; i++) {
		str[0] = airstr[scp->airport[i].dir];
		str[1] = '0' + i;
		wmove(radar, scp->airport[i].y, scp->airport[i].x * 2);
		waddstr(radar, str);
	}

	wattrset(radar,0);
	overwrite(radar, cleanradar);
	wrefresh(radar);
	wrefresh(credit);
	fflush(stdout);
}

void
draw_line(w, x, y, lx, ly, s)
	WINDOW	*w;
	int	 x, y, lx, ly;
	const char	*s;
{
	int	dx, dy;

	dx = SGN(lx - x);
	dy = SGN(ly - y);
	for (;;) {
		wmove(w, y, x * 2);
		waddstr(w, s);
		if (x == lx && y == ly)
			break;
		x += dx;
		y += dy;
	}
}

void
ioclrtoeol(pos)
	int pos;
{
	wmove(input, 0, pos);
	wclrtoeol(input);
	wrefresh(input);
	fflush(stdout);
}

void
iomove(pos)
	int pos;
{
	wmove(input, 0, pos);
	wrefresh(input);
	fflush(stdout);
}

void
ioaddstr(pos, str)
	int	 pos;
	const char	*str;
{
	wmove(input, 0, pos);
	waddstr(input, str);
	wrefresh(input);
	fflush(stdout);
}

void
ioclrtobot()
{
	wclrtobot(input);
	wrefresh(input);
	fflush(stdout);
}

void
ioerror(pos, len, str)
	int	 pos, len;
	const char	*str;
{
	int	i;

	wmove(input, 1, pos);
	for (i = 0; i < len; i++)
		waddch(input, '^');
	wmove(input, 2, 0);
	waddstr(input, str);
	wrefresh(input);
	fflush(stdout);
}

void
quit(dummy)
	int dummy __attribute__((__unused__));
{
	int			c, y, x;
#ifdef BSD
	struct itimerval	itv;
#endif

	getyx(input, y, x);
	wmove(input, 2, 0);
	waddstr(input, "Really quit? (y/n) ");
	wclrtobot(input);
	wrefresh(input);
	fflush(stdout);

	c = getchar();
	if (c == EOF || c == 'y') {
		/* disable timer */
#ifdef BSD
		itv.it_value.tv_sec = 0;
		itv.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
		alarm(0);
#endif
		fflush(stdout);
		clear();
		refresh();
		endwin();
		log_score(0);
		exit(0);
	}
	wmove(input, 2, 0);
	wclrtobot(input);
	wmove(input, y, x);
	wrefresh(input);
	fflush(stdout);
}

void
planewin()
{
	PLANE	*pp;
	int	warning = 0;
	char curcmd[20];

#ifdef BSD
	werase(planes);
#endif

	wmove(planes, 0,0);

#ifdef SYSV
	wclrtobot(planes);
#endif
  wprintw(planes, "Time: ");
	wattrset(planes, A_BOLD);
	wprintw(planes, "%-4d", clck);
	wattrset(planes, 0);
	wprintw(planes, " Safe: ");
	wattrset(planes, A_BOLD);
	wprintw(planes, "%d", safe_planes);
	wattrset(planes, 0);
	wmove(planes, 2, 0);

  wattron(planes, COLOR_PAIR(COL_HEAD)|A_BOLD);
	waddstr(planes, "pl dt  comm"); wattrset(planes, 0);
	for (pp = air.head; pp != NULL; pp = pp->next) {
		if (waddch(planes, '\n') == ERR) {
			warning++;
			break;
		}
		sprintf(curcmd,"%s",command(pp));
// In the plane list match the plane color to the one on the radar
		if ((curcmd[0] > 64) && (curcmd[0] < 91)) {
			wattron(planes, COLOR_PAIR(COL_PL_PR_1)|A_BOLD);
		} else {
			wattron(planes, COLOR_PAIR(COL_PL_JT_1)|A_BOLD);
		}
		waddch(planes, curcmd[0]); waddch(planes, curcmd[1]);
		wattron(planes, COLOR_PAIR(COL_FUEL));
		waddch(planes, curcmd[2]);
// In the plane list match the destination color to the one on the radar
		if (curcmd[3] == 'A') {
			wattron(planes, COLOR_PAIR(COL_AIRPT));
		} else {
			wattron(planes, COLOR_PAIR(COL_EXIT));
		}
		waddch(planes, curcmd[3]); waddch(planes, curcmd[4]);
		wattrset(planes, 0);
		waddstr(planes, curcmd+5);
	}
	waddch(planes, '\n');
	for (pp = ground.head; pp != NULL; pp = pp->next) {
		if (waddch(planes, '\n') == ERR) {
			warning++;
			break;
		}
		sprintf(curcmd,"%s",command(pp));
		if ((curcmd[0] > 64) && (curcmd[0] < 91)) {
			wattron(planes, COLOR_PAIR(COL_PL_PR_1)|A_BOLD);
		} else {
			wattron(planes, COLOR_PAIR(COL_PL_JT_1)|A_BOLD);
		}
		waddch(planes, curcmd[0]); waddch(planes, curcmd[1]);
		wattron(planes, COLOR_PAIR(COL_FUEL));
		waddch(planes, curcmd[2]);
		if (curcmd[3] == 'A') {
			wattron(planes, COLOR_PAIR(COL_AIRPT));
		} else {
			wattron(planes, COLOR_PAIR(COL_EXIT));
		}
		waddch(planes, curcmd[3]); waddch(planes, curcmd[4]);
		wattrset(planes, 0);
		waddstr(planes, curcmd+5);
	}
	if (warning) {
		wmove(planes, LINES - INPUT_LINES - 1, 0);
		waddstr(planes, "---- more ----");
		wclrtoeol(planes);
	}
	wrefresh(planes);
	fflush(stdout);
}

void
loser(p, s)
	const PLANE	*p;
	const char	*s;
{
	int			c;
#ifdef BSD
	struct itimerval	itv;
#endif

	/* disable timer */
#ifdef BSD
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
	alarm(0);
#endif

	wmove(input, 0, 0);
	wclrtobot(input);
	/* p may be NULL if we ran out of memory */
	if (p == NULL)
		wprintw(input, "%s\n\nHit space for top players list...", s);
	else
		wprintw(input, "Plane '%c' %s\n\nHit space for top players list...",
			name(p), s);
	wrefresh(input);
	fflush(stdout);
	while ((c = getchar()) != EOF && c != ' ')
		;
	clear();	/* move to top of screen */
	refresh();
	endwin();
	log_score(0);
	exit(0);
}

void
redraw()
{
	clear();
	refresh();

	touchwin(radar);
	wrefresh(radar);
	touchwin(planes);
	wrefresh(planes);
	touchwin(credit);
	wrefresh(credit);

	/* refresh input last to get cursor in right place */
	touchwin(input);
	wrefresh(input);
	fflush(stdout);
}

void
done_screen()
{
	clear();
	refresh();
	endwin();	  /* clean up curses */
}
