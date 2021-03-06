// RUN: %clang_cc1 %s -emit-llvm -o %t.ll -triple=x86_64-apple-darwin10
// RUN: FileCheck %s < %t.ll
// RUN: FileCheck -check-prefix=CHECK-GLOBAL %s < %t.ll
// RUN: %clang_cc1 %s -emit-llvm -o %t-opt.ll -triple=x86_64-apple-darwin10 -O3
// RUN: FileCheck --check-prefix=CHECK-O3 %s < %t-opt.ll

struct A { int a; int b; };
struct B { int b; };
struct C : B, A { };

// Zero init.
namespace ZeroInit {
  // CHECK-GLOBAL: @_ZN8ZeroInit1aE = global i64 -1
  int A::* a;
  
  // CHECK-GLOBAL: @_ZN8ZeroInit2aaE = global [2 x i64] [i64 -1, i64 -1]
  int A::* aa[2];
  
  // CHECK-GLOBAL: @_ZN8ZeroInit3aaaE = global [2 x [2 x i64]] {{\[}}[2 x i64] [i64 -1, i64 -1], [2 x i64] [i64 -1, i64 -1]]
  int A::* aaa[2][2];
  
  // CHECK-GLOBAL: @_ZN8ZeroInit1bE = global i64 -1,
  int A::* b = 0;

  // CHECK-GLOBAL: @_ZN8ZeroInit2saE = internal global %"struct._ZN8ZeroInit3$_0E" { i64 -1 }
  struct {
    int A::*a;
  } sa;
  void test_sa() { (void) sa; } // force emission
  
  // CHECK-GLOBAL: @_ZN8ZeroInit3ssaE = internal
  // CHECK-GLOBAL: [2 x i64] [i64 -1, i64 -1]
  struct {
    int A::*aa[2];
  } ssa[2];
  void test_ssa() { (void) ssa; }
  
  // CHECK-GLOBAL: @_ZN8ZeroInit2ssE = internal global %"struct._ZN8ZeroInit3$_2E" { %"struct._ZN8ZeroInit3$_23$_3E" { i64 -1 } }
  struct {
    struct {
      int A::*pa;
    } s;
  } ss;
  void test_ss() { (void) ss; }
  
  struct A {
    int A::*a;
    int b;
  };

  struct B {
    A a[10];
    char c;
    int B::*b;
  };

  struct C : A, B { int j; };
  // CHECK-GLOBAL: @_ZN8ZeroInit1cE = global {{%.*}} <{ %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1BE { [10 x %struct._ZN8ZeroInit1AE] [%struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }, %struct._ZN8ZeroInit1AE { i64 -1, i32 0 }], i8 0, i64 -1 }, i32 0, [4 x i8] zeroinitializer }>, align 8
  C c;
}

// PR5674
namespace PR5674 {
  // CHECK-GLOBAL: @_ZN6PR56742pbE = global i64 4
  int A::*pb = &A::b;
}

// Casts.
namespace Casts {

int A::*pa;
int C::*pc;

void f() {
  // CHECK:      store i64 -1, i64* @_ZN5Casts2paE
  pa = 0;

  // CHECK-NEXT: [[TMP:%.*]] = load i64* @_ZN5Casts2paE, align 8
  // CHECK-NEXT: [[ADJ:%.*]] = add nsw i64 [[TMP]], 4
  // CHECK-NEXT: [[ISNULL:%.*]] = icmp eq i64 [[TMP]], -1
  // CHECK-NEXT: [[RES:%.*]] = select i1 [[ISNULL]], i64 [[TMP]], i64 [[ADJ]]
  // CHECK-NEXT: store i64 [[RES]], i64* @_ZN5Casts2pcE
  pc = pa;

  // CHECK-NEXT: [[TMP:%.*]] = load i64* @_ZN5Casts2pcE, align 8
  // CHECK-NEXT: [[ADJ:%.*]] = sub nsw i64 [[TMP]], 4
  // CHECK-NEXT: [[ISNULL:%.*]] = icmp eq i64 [[TMP]], -1
  // CHECK-NEXT: [[RES:%.*]] = select i1 [[ISNULL]], i64 [[TMP]], i64 [[ADJ]]
  // CHECK-NEXT: store i64 [[RES]], i64* @_ZN5Casts2paE
  pa = static_cast<int A::*>(pc);
}

}

// Comparisons
namespace Comparisons {
  void f() {
    int A::*a;

    // CHECK: icmp ne i64 {{.*}}, -1
    if (a) { }

    // CHECK: icmp ne i64 {{.*}}, -1
    if (a != 0) { }
    
    // CHECK: icmp ne i64 -1, {{.*}}
    if (0 != a) { }

    // CHECK: icmp eq i64 {{.*}}, -1
    if (a == 0) { }

    // CHECK: icmp eq i64 -1, {{.*}}
    if (0 == a) { }
  }
}

namespace ValueInit {

struct A {
  int A::*a;

  char c;

  A();
};

// CHECK-LABEL: define void @_ZN9ValueInit1AC2Ev(%struct._ZN9ValueInit1AE* %this) unnamed_addr
// CHECK: store i64 -1, i64*
// CHECK: ret void
A::A() : a() {}

}

namespace PR7139 {

struct pair {
  int first;
  int second;
};

typedef int pair::*ptr_to_member_type;

struct ptr_to_member_struct { 
  ptr_to_member_type data;
  int i;
};

struct A {
  ptr_to_member_struct a;

  A() : a() {}
};

// CHECK-O3: define zeroext i1 @_ZN6PR71395checkEv() [[NUW:#[0-9]+]]
bool check() {
  // CHECK-O3: ret i1 true
  return A().a.data == 0;
}

// CHECK-O3: define zeroext i1 @_ZN6PR71396check2Ev() [[NUW]]
bool check2() {
  // CHECK-O3: ret i1 true
  return ptr_to_member_type() == 0;
}

}

namespace VirtualBases {

struct A {
  char c;
  int A::*i;
};

// CHECK-GLOBAL: @_ZN12VirtualBases1bE = global %struct._ZN12VirtualBases1BE { i32 (...)** null, %struct._ZN12VirtualBases1AE { i8 0, i64 -1 } }, align 8
struct B : virtual A { };
B b;

// CHECK-GLOBAL: @_ZN12VirtualBases1cE = global %struct._ZN12VirtualBases1CE { i32 (...)** null, i64 -1, %struct._ZN12VirtualBases1AE { i8 0, i64 -1 } }, align 8
struct C : virtual A { int A::*i; };
C c;

// CHECK-GLOBAL: @_ZN12VirtualBases1dE = global %struct._ZN12VirtualBases1DE { %struct._ZN12VirtualBases1CE.base { i32 (...)** null, i64 -1 }, i64 -1, %struct._ZN12VirtualBases1AE { i8 0, i64 -1 } }, align 8
struct D : C { int A::*i; };
D d;

}

namespace Test1 {

// Don't crash when A contains a bit-field.
struct A {
  int A::* a;
  int b : 10;
};
A a;

}

namespace BoolPtrToMember {
  struct X {
    bool member;
  };

  // CHECK-LABEL: define dereferenceable({{[0-9]+}}) i8* @_ZN15BoolPtrToMember1fERNS_1XEMS0_b
  bool &f(X &x, bool X::*member) {
    // CHECK: {{bitcast.* to i8\*}}
    // CHECK-NEXT: getelementptr inbounds i8*
    // CHECK-NEXT: ret i8*
    return x.*member;
  }
}

namespace PR8507 {
  
struct S;
void f(S* p, double S::*pm) {
  if (0 < p->*pm) {
  }
}

}

namespace test4 {
  struct A             { int A_i; };
  struct B : virtual A { int A::*B_p; };
  struct C : virtual B { int    *C_p; };
  struct D :         C { int    *D_p; };

  // CHECK-GLOBAL: @_ZN5test41dE = global %struct._ZN5test41DE { %struct._ZN5test41CE.base zeroinitializer, i32* null, %struct._ZN5test41BE.base { i32 (...)** null, i64 -1 }, %struct._ZN5test41AE zeroinitializer }, align 8
  D d;
}

namespace PR11487 {
  union U
  {
    int U::* mptr;
    char x[16];
  } x;
  // CHECK-GLOBAL: @_ZN7PR114871xE = global %union._ZN7PR114871UE { i64 -1, [8 x i8] zeroinitializer }, align 8
  
}

namespace PR13097 {
  struct X { int x; X(const X&); };
  struct A {
    int qq;
      X x;
  };
  A f();
  X g() { return f().*&A::x; }
  // CHECK-LABEL: define void @_ZN7PR130971gEv
  // CHECK: call void @_ZN7PR130971fEv
  // CHECK-NOT: memcpy
  // CHECK: call void @_ZN7PR130971XC1ERKS0_
}

namespace PR21089 {
struct A {
  bool : 1;
  int A::*x;
  bool y;
  A();
};
struct B : A {
};
B b;
// CHECK-GLOBAL: @_ZN7PR210891bE = global %struct._ZN7PR210891BE { %struct._ZN7PR210891AE.base <{ i8 0, [7 x i8] zeroinitializer, i64 -1, i8 0 }>, [7 x i8] zeroinitializer }, align 8
}

namespace PR21282 {
union U {
  int U::*x;
  long y[2];
};
U u;
// CHECK-GLOBAL: @_ZN7PR212821uE = global %union._ZN7PR212821UE { i64 -1, [8 x i8] zeroinitializer }, align 8
}

// CHECK-O3: attributes [[NUW]] = { nounwind readnone{{.*}} }
