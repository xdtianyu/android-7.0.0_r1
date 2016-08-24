/*
 * Copyright (C) 2014 Satoshi Noguchi
 * Copyright (C) 2014 Synaptics Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#include "display.h"

#define ESC 0x1B

// default display
void Display::Output(const char * buf)
{
	printf("%s", buf);
}

// ansi console
AnsiConsole::AnsiConsole() : Display()
{
	m_buf = NULL;
	GetWindowSize();
	m_curX = 0;
	m_curY = 0;
	m_maxCurX = 0;
	m_maxCurY = 0;
}

AnsiConsole::~AnsiConsole()
{
	delete [] m_buf;
}

void AnsiConsole::GetWindowSize()
{
	struct winsize winsz;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz);
	if (m_numRows != winsz.ws_row || m_numCols != winsz.ws_col)
	{
		m_numRows = winsz.ws_row;
		m_numCols = winsz.ws_col;
		if (m_buf != NULL) 
		{
			delete [] m_buf;
		}
		m_buf = new char[m_numRows * m_numCols];

		Clear();
	}
}

void AnsiConsole::Output(const char * buf)
{
	char * p;

	while (m_curY < m_numRows &&
		m_numCols * m_curY + m_curX < m_numRows * m_numCols)
	{
		p = &(m_buf[m_numCols * m_curY + m_curX]);

		if (*buf == '\0')
		{
			break;
		}
		else if (*buf == '\n')
		{
			memset(p, ' ', m_numCols - m_curX);
			m_curX = 0;
			m_curY++;
		}			
		else if (m_curX < m_numCols)
		{
			*p = *buf;
			m_curX++;
		}
		buf++;

		if (m_maxCurX < m_curX) m_maxCurX = m_curX;
		if (m_maxCurY < m_curY) m_maxCurY = m_curY;
	}
}

void AnsiConsole::Clear()
{
	printf("%c[2J", ESC);
}

void AnsiConsole::Reflesh()
{
	int i;
	int j;
	char * p;

	printf("%c[%d;%dH", ESC, 0, 0);

	for (j = 0; j < m_maxCurY; j++)
	{
		p = &(m_buf[m_numCols * j]);

		for (i = 0; i < m_maxCurX; i++)
		{
			putc(*p, stdout);
			p++;
		}

		putc('\n', stdout);
	}

	GetWindowSize();
	m_curX = 0;
	m_curY = 0;
	m_maxCurX = 0;
	m_maxCurY = 0;
}
