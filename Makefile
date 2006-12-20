#
# FontView Makefile for building the app.
# Part of the Fontable Project
# Copyright (C) 2006 Jon Phillips, <jon@rejon.org>.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or 
#  (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License 
# along with this program; if not, write to the Free Software 
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
# MA 02111-1307 USA
#

CC=gcc
CFLAGS=-c -Wall
LDFLAGS=`pkg-config --libs --cflags gtk+-2.0 pango freetype2`

SRC=main.c font-model.c font-view.c
# OBJ=$(SRC:.c=.o)
EXECUTABLE=fv


all: $(SRC) $(EXECUTABLE)

$(EXECUTABLE): $(SRC)
	$(CC) $(SRC) -o $@ $(LDFLAGS)

#%.o: %.c
#	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -Rf $(OBJ) $(EXECUTABLE)

.PHONY: clean
