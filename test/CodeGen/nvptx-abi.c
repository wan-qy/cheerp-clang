// RUN: %clang_cc1 -triple nvptx-unknown-unknown -S -o - %s -emit-llvm | FileCheck %s
// RUN: %clang_cc1 -triple nvptx64-unknown-unknown -S -o - %s -emit-llvm | FileCheck %s

typedef struct float4_s {
  float x, y, z, w;
} float4_t;

float4_t my_function(void) {
// CHECK-LABEL: define %struct._Z8float4_s @my_function
  float4_t t;
  return t;
};

float bar(void) {
  float4_t ret;
// CHECK-LABEL: @bar
// CHECK: call %struct._Z8float4_s @my_function
  ret = my_function();
  return ret.x;
}

void foo(float4_t x) {
// CHECK-LABEL: @foo
// CHECK: %struct._Z8float4_s* byval %x
}

void fooN(float4_t x, float4_t y, float4_t z) {
// CHECK-LABEL: @fooN
// CHECK: %struct._Z8float4_s* byval %x
// CHECK: %struct._Z8float4_s* byval %y
// CHECK: %struct._Z8float4_s* byval %z
}

typedef struct nested_s {
  unsigned long long x;
  float z[64];
  float4_t t;
} nested_t;

void baz(nested_t x) {
// CHECK-LABEL: @baz
// CHECK: %struct._Z8nested_s* byval %x)
}
