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

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

class Display
{
public:
	Display() {}
	virtual ~Display() {}

	virtual void Clear() {};
	virtual void Reflesh() {};
	virtual void Output(const char * buf);
};

class AnsiConsole : public Display
{
public:
	AnsiConsole();
	virtual ~AnsiConsole();

	virtual void Clear();
	virtual void Reflesh();
	virtual void Output(const char * buf);

private:
	void GetWindowSize();

protected:
	int m_numCols;
	int m_numRows;
	int m_curX;
	int m_curY;
	int m_maxCurX;
	int m_maxCurY;
	char * m_buf;
};

#endif // _DISPLAY_H_
