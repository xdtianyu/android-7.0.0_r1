/*
 * Copyright (C) 2014 Andrew Duggan
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

#ifndef _RMIFUNCTION_H_
#define _RMIFUNCTION_H_

class RMIFunction
{
public:
	RMIFunction() {}
	RMIFunction(const unsigned char * pdtEntry, unsigned short pageBase, unsigned int interruptCount);
	unsigned short GetQueryBase() { return m_queryBase; }
	unsigned short GetCommandBase() { return m_commandBase; }
	unsigned short GetControlBase() { return m_controlBase; }
	unsigned short GetDataBase() { return m_dataBase; }
	unsigned char GetInterruptSourceCount() { return m_interruptSourceCount; }
	unsigned char GetFunctionNumber() { return m_functionNumber; }
	unsigned char GetFunctionVersion() { return m_functionVersion; }
	unsigned char GetInterruptRegNum() { return m_interruptRegNum; }
	unsigned char GetInterruptMask() { return m_interruptMask; }

private:
	unsigned short m_queryBase;
	unsigned short m_commandBase;
	unsigned short m_controlBase;
	unsigned short m_dataBase;
	unsigned char m_interruptSourceCount;
	unsigned char m_functionNumber;
	unsigned char m_functionVersion;
	unsigned char m_interruptRegNum;
	unsigned char m_interruptMask;
};

#endif // _RMIFUNCTION_H_