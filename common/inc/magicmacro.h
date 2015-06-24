#ifndef __MAGIC_MACRO__
#define __MAGIC_MACRO__

#ifdef pad_always

#define pad(p,f) p##0

#else

#define pad0(p,not_used) p
#define pad1(p,not_used) p##0

#define pad(p,f) pad##f(p,)

#endif

// f - padding flag
// p - prefix so far
// a,b,c - digits
// x - action to invoke

#define n0(p,x)
#define n1(p,x)         x(p##1)
#define n2(p,x) n1(p,x) x(p##2)
#define n3(p,x) n2(p,x) x(p##3)
#define n4(p,x) n3(p,x) x(p##4)
#define n5(p,x) n4(p,x) x(p##5)
#define n6(p,x) n5(p,x) x(p##6)
#define n7(p,x) n6(p,x) x(p##7)
#define n8(p,x) n7(p,x) x(p##8)
#define n9(p,x) n8(p,x) x(p##9)

#define n00(f,p,a,x)                       n##a(pad(p,f),x)
#define n10(f,p,a,x) n00(f,p,9,x) x(p##10) n##a(p##1,x)
#define n20(f,p,a,x) n10(f,p,9,x) x(p##20) n##a(p##2,x)
#define n30(f,p,a,x) n20(f,p,9,x) x(p##30) n##a(p##3,x)
#define n40(f,p,a,x) n30(f,p,9,x) x(p##40) n##a(p##4,x)
#define n50(f,p,a,x) n40(f,p,9,x) x(p##50) n##a(p##5,x)
#define n60(f,p,a,x) n50(f,p,9,x) x(p##60) n##a(p##6,x)
#define n70(f,p,a,x) n60(f,p,9,x) x(p##70) n##a(p##7,x)
#define n80(f,p,a,x) n70(f,p,9,x) x(p##80) n##a(p##8,x)
#define n90(f,p,a,x) n80(f,p,9,x) x(p##90) n##a(p##9,x)

#define n000(f,p,a,b,x)                           n##a##0(f,pad(p,f),b,x)
#define n100(f,p,a,b,x) n000(f,p,9,9,x) x(p##100) n##a##0(1,p##1,b,x)
#define n200(f,p,a,b,x) n100(f,p,9,9,x) x(p##200) n##a##0(1,p##2,b,x)
#define n300(f,p,a,b,x) n200(f,p,9,9,x) x(p##300) n##a##0(1,p##3,b,x)
#define n400(f,p,a,b,x) n300(f,p,9,9,x) x(p##400) n##a##0(1,p##4,b,x)
#define n500(f,p,a,b,x) n400(f,p,9,9,x) x(p##500) n##a##0(1,p##5,b,x)
#define n600(f,p,a,b,x) n500(f,p,9,9,x) x(p##600) n##a##0(1,p##6,b,x)
#define n700(f,p,a,b,x) n600(f,p,9,9,x) x(p##700) n##a##0(1,p##7,b,x)
#define n800(f,p,a,b,x) n700(f,p,9,9,x) x(p##800) n##a##0(1,p##8,b,x)
#define n900(f,p,a,b,x) n800(f,p,9,9,x) x(p##900) n##a##0(1,p##9,b,x)

#define n0000(f,p,a,b,c,x)                               n##a##00(f,pad(p,f),b,c,x)
#define n1000(f,p,a,b,c,x) n0000(f,p,9,9,9,x) x(p##1000) n##a##00(1,p##1,b,c,x)
#define n2000(f,p,a,b,c,x) n1000(f,p,9,9,9,x) x(p##2000) n##a##00(1,p##2,b,c,x)
#define n3000(f,p,a,b,c,x) n2000(f,p,9,9,9,x) x(p##3000) n##a##00(1,p##3,b,c,x)
#define n4000(f,p,a,b,c,x) n3000(f,p,9,9,9,x) x(p##4000) n##a##00(1,p##4,b,c,x)
#define n5000(f,p,a,b,c,x) n4000(f,p,9,9,9,x) x(p##5000) n##a##00(1,p##5,b,c,x)
#define n6000(f,p,a,b,c,x) n5000(f,p,9,9,9,x) x(p##6000) n##a##00(1,p##6,b,c,x)
#define n7000(f,p,a,b,c,x) n6000(f,p,9,9,9,x) x(p##7000) n##a##00(1,p##7,b,c,x)
#define n8000(f,p,a,b,c,x) n7000(f,p,9,9,9,x) x(p##8000) n##a##00(1,p##8,b,c,x)
#define n9000(f,p,a,b,c,x) n8000(f,p,9,9,9,x) x(p##9000) n##a##00(1,p##9,b,c,x)

#define n00000(f,p,a,b,c,d,x)                                   n##a##000(f,pad(p,f),b,c,d,x)
#define n10000(f,p,a,b,c,d,x) n00000(f,p,9,9,9,9,x) x(p##10000) n##a##000(1,p##1,b,c,d,x)
#define n20000(f,p,a,b,c,d,x) n10000(f,p,9,9,9,9,x) x(p##20000) n##a##000(1,p##2,b,c,d,x)
#define n30000(f,p,a,b,c,d,x) n20000(f,p,9,9,9,9,x) x(p##30000) n##a##000(1,p##3,b,c,d,x)
#define n40000(f,p,a,b,c,d,x) n30000(f,p,9,9,9,9,x) x(p##40000) n##a##000(1,p##4,b,c,d,x)
#define n50000(f,p,a,b,c,d,x) n40000(f,p,9,9,9,9,x) x(p##50000) n##a##000(1,p##5,b,c,d,x)
#define n60000(f,p,a,b,c,d,x) n50000(f,p,9,9,9,9,x) x(p##60000) n##a##000(1,p##6,b,c,d,x)
#define n70000(f,p,a,b,c,d,x) n60000(f,p,9,9,9,9,x) x(p##70000) n##a##000(1,p##7,b,c,d,x)
#define n80000(f,p,a,b,c,d,x) n70000(f,p,9,9,9,9,x) x(p##80000) n##a##000(1,p##8,b,c,d,x)
#define n90000(f,p,a,b,c,d,x) n80000(f,p,9,9,9,9,x) x(p##90000) n##a##000(1,p##9,b,c,d,x)

#define cycle5(c1,c2,c3,c4,c5,x) n##c1##0000(0,,c2,c3,c4,c5,x)
#define cycle4(c1,c2,c3,c4,x) n##c1##000(0,,c2,c3,c4,x)
#define cycle3(c1,c2,c3,x) n##c1##00(0,,c2,c3,x)
#define cycle2(c1,c2,x) n##c1##0(0,,c2,x)
#define cycle1(c1,x) n##c1(,x)

#define concat(a,b,c) a##b##c


#endif