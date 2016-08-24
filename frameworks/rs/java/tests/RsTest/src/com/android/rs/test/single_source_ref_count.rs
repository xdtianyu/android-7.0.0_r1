#include "shared.rsh"

static const int dimX = 1024;
static const int dimY = 768;
static const int dimZ = 0;

// Tests an object returned via a local variable is valid to the caller.
static rs_allocation test1() {
  rs_allocation retValue = {};
  rs_element element = rsCreateVectorElement(RS_TYPE_FLOAT_32, 4);
  if (!rsIsObject(element)) {
    rsDebug("element is null.", element.p);
    return retValue;
  }

  rs_type type = rsCreateType(element, dimX, dimY, dimZ);
  if (!rsIsObject(type)) {
    rsDebug("type is null.", type.p);
    return retValue;
  }

  retValue = rsCreateAllocation(type);
  if (!rsIsObject(retValue)) {
    rsDebug("rs_allocation retValue is null.", retValue.p);
  }

  return retValue;
}

// Tests an object returned via an expression is valid to the caller.
static rs_allocation test2() {
  rs_allocation empty = {};
  rs_element element = rsCreateVectorElement(RS_TYPE_FLOAT_32, 4);
  if (!rsIsObject(element)) {
    rsDebug("element is null.", element.p);
    return empty;
  }

  rs_type type = rsCreateType(element, dimX, dimY, dimZ);
  if (!rsIsObject(type)) {
    rsDebug("type is null.", type.p);
    return empty;
  }

  return rsCreateAllocation(type);
}

static struct testS {
  rs_allocation (*fp)();
  const char* name;
} tests[] = {
  { test1, "test1" },
  { test2, "test2" },
  { NULL,  NULL    }
};

void entrypoint() {
  int failed = 0;

  for (int i = 0; tests[i].fp; i++) {
    rsDebug(tests[i].name, 0);
    rs_allocation allocation = tests[i].fp();
    if (!rsIsObject(allocation)) {
      failed++;
      rsDebug("failed.", 0);
    } else {
      rsDebug("passed.", 0);
    }
  }

  if (failed) {
    rsDebug("Reference counting tests failed: ", failed);
    rsSendToClientBlocking(RS_MSG_TEST_FAILED);
  } else {
    rsDebug("All reference counting tests passed.", 0);
    rsSendToClientBlocking(RS_MSG_TEST_PASSED);
  }
}
