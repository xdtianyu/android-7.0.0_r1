// RUN: %clang_cc1 %s -verify -fsyntax-only

int __attribute__((kernel)) g;               // expected-warning {{'kernel' attribute only applies to functions}}

int __attribute__((kernel)) f1(void) {       // expected-warning {{'kernel' attribute ignored}}
  return 0;
}

int __attribute__((kernel(1, 2))) f2(void) { // expected-error {{'kernel' attribute takes no more than 1 argument}}
  return 0;
}
