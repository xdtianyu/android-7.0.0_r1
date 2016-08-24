/*
 * Copyright (C) 2015 The Android Open Source Project
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
/* Add firmware update command to recovery script */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "edify/expr.h"
#include "update_fw.h"

Value* firmware_update(const char *name, State * state, int argc, Expr * argv[]) {
	Value *firmware;
	Value *ec;
	int res;
	Value *retval = NULL;

	printf("%s: running %s.\n", __func__, name);
	if (argc < 2) {
		ErrorAbort(state, "syntax: %s bios.bin ec.bin", name);
		return NULL;
	}
	if (ReadValueArgs(state, argv, 2, &firmware, &ec) < 0) {
		ErrorAbort(state, "%s: invalid arguments", name);
		return NULL;
	}

	res = update_fw(firmware, ec, 0);
	if (res < 0)
		ErrorAbort(state, "%s: firmware update error", name);
	else
		retval = StringValue(strdup(res ? "UPDATED" : ""));

	FreeValue(firmware);
	FreeValue(ec);

	printf("%s: [%s] done.\n", __func__,
		retval ? retval->data : state->errmsg);
	return retval;
}

void Register_librecovery_updater_dragon() {
	RegisterFunction("dragon.firmware_update", firmware_update);
}
