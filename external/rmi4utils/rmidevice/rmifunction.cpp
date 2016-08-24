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

#include "rmifunction.h"

#define RMI_FUNCTION_QUERY_OFFSET		0
#define RMI_FUNCTION_COMMAND_OFFSET		1
#define RMI_FUNCTION_CONTROL_OFFSET		2
#define RMI_FUNCTION_DATA_OFFSET		3
#define RMI_FUNCTION_INTERRUPT_SOURCES_OFFSET	4
#define RMI_FUNCTION_NUMBER			5

#define RMI_FUNCTION_VERSION_MASK		0x60
#define RMI_FUNCTION_INTERRUPT_SOURCES_MASK	0x7

RMIFunction::RMIFunction(const unsigned char * pdtEntry, unsigned short pageBase, unsigned int interruptCount)
{
	unsigned char ii;
	unsigned char interruptOffset;

	if (pdtEntry) {
		m_queryBase = pdtEntry[RMI_FUNCTION_QUERY_OFFSET] + pageBase;
		m_commandBase = pdtEntry[RMI_FUNCTION_COMMAND_OFFSET] + pageBase;
		m_controlBase = pdtEntry[RMI_FUNCTION_CONTROL_OFFSET] + pageBase;
		m_dataBase = pdtEntry[RMI_FUNCTION_DATA_OFFSET] + pageBase;
		m_interruptSourceCount = pdtEntry[RMI_FUNCTION_INTERRUPT_SOURCES_OFFSET]
						& RMI_FUNCTION_INTERRUPT_SOURCES_MASK;
		m_functionNumber = pdtEntry[RMI_FUNCTION_NUMBER];
		m_functionVersion = (pdtEntry[RMI_FUNCTION_INTERRUPT_SOURCES_OFFSET]
						& RMI_FUNCTION_VERSION_MASK) >> 5;
		if (m_interruptSourceCount > 0)
		{
			m_interruptRegNum = (interruptCount + 8) / 8 - 1;

			/* Set an enable bit for each data source */
			interruptOffset = interruptCount % 8;
			m_interruptMask = 0;
			for (ii = interruptOffset;
					ii < (m_interruptSourceCount + interruptOffset);
					ii++)
				m_interruptMask |= 1 << ii;
		}
	}
}