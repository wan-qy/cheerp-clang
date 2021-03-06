// RUN: %clang_cc1 -triple i386-unknown-unknown -emit-llvm -o - %s | FileCheck %s

void __attribute__((fastcall)) foo1(int &y);
void bar1(int &y) {
  // CHECK-LABEL: define void @_Z4bar1Ri
  // CHECK: call x86_fastcallcc void @_Z4foo1Ri(i32* inreg dereferenceable({{[0-9]+}}) %
  foo1(y);
}

struct S1 {
  int x;
  S1(const S1 &y);
};

void __attribute__((fastcall)) foo2(S1 a, int b);
void bar2(S1 a, int b) {
  // CHECK-LABEL: define void @_Z4bar22S1i
  // CHECK: call x86_fastcallcc void @_Z4foo22S1i(%struct._Z2S1* inreg %{{.*}}, i32 inreg %
  foo2(a, b);
}
