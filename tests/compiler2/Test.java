class Test {

static long c = 0;

public static void main(String arg[]) {
  System.out.println(fib(15));
  System.out.println(OverflowLong(15));
  System.out.println(Long.MIN_VALUE+14);
  System.out.println(OverflowInt(15));
  System.out.println(Integer.MIN_VALUE+14);
  System.out.println(OverflowLongIntLong(15));
  System.out.println(Integer.MIN_VALUE+14);
}

static double mul(double a, double b) {
    return a * b;
}

static double div(double a, double b) {
    return a / b;
}

static long fib(long n) {
  //System.out.println(++c);
  if (n < 1 )
    return -1;
  if (n == 1)
    return 1;
  if (n == 2)
    return 1;
  return fib(n-1) + fib(n-2);
}

static long fib2(long n,long a) {
  //System.out.println(++c);
  if (n < 1 )
    return -1;
  if (n == 1)
    return 1;
  if (n == 2)
    return 1;
  return fib(n-1) + fib(n-2) + a;
}

static long return_input(long n) {
    return n;
}

static long fact(long n) {

    long res = 1;
    while (1 < n) {
        res *= n--;
    }
    return res;
}

static long OverflowLong(long n) {
    return n + Long.MAX_VALUE;
}

static long OverflowLongIntLong(long n) {
    int y = 1;
    y += Integer.MAX_VALUE;
    y -= Integer.MIN_VALUE;
    return n + y;
}

static int OverflowInt(int n) {
    return n + Integer.MAX_VALUE;
}

static long test2(long n,long a) {
  while (n > a) {
    n--;
    a++;
  }
  return a+n;
}

static long testParameterLong(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    return a+b+c+d+e+f+g+h+i;
}
static long testParameterLong2(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    return a+c+e+g+i;
}
static long testParameterLong3(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    return b+d+f+h;
}
static int testParameterInt(int a, int b, int c, int d, int e, int f, int g, int h, int i) {
    return a+b+c+d+e+f+g+h+i;
}
static float testParameterFloat(float a, float b, float c, float d, float e, float f, float g, float h, float i) {
    return a+b+c+d+e+f+g+h+i;
}
static double testParameterDouble(double a, double b, double c, double d, double e, double f, double g, double h, double i) {
    return a+b+c+d+e+f+g+h+i;
}
static double testParameterMixed(long a, float b, double c, float d, float e, int f, double g, int h, long i) {
    return a+b+c+d+e+f+g+h+i;
}

static long loop2(long n,long a) {
  while (n > a) {
    n--;
    while(n > a) {
      n--;
    }
    while(n < a) {
      a++;
    }
    a++;
  }
  return a+n;
}

//static long loop3(long n,long a, int c, long d, int f) {
static int loop3(int n,int a, int c, int d, int f) {
  while (n > a) {   // S
    a++;
    if (c > d) {
      while (c!=0) {// B
        d++;
        switch (c) {
          case 0:   // A
            f++;
          case 1:   // D
            f++;
            break;
          default:  // E
            f--;
        }
        c++;        // H
      }
    } else {        // C
      if (a!=0) {   // F
        a--;
      } else{       // G
        if (f!=0) { // J
          f--;
        }
      }
      n--;          // I
      continue;
    }
  }
  return a+n+c+d+f;
}

//static long loop3(long n,long a, int c, long d, int f) {
static int loop4(int n,int a, int c, int d, int f) {
  while ( a == 1) {
    a++;
    while (n > a) {   // S
      a++;
      if (c > d) {
        while (c!=0) {// B
          d++;
          switch (c) {
            case 0:   // A
              f++;
            case 1:   // D
              f++;
              break;
            default:  // E
              f--;
          }
          c++;        // H
        }
      } else {        // C
        if (a!=0) {   // F
          a--;
        } else{       // G
          if (f!=0) { // J
            f--;
          }
        }
        n--;          // I
        continue;
      }
    }
  }
  return a+n+c+d+f;
}

static void collatz(int n) {
  while( n != 0) {
    if ((n & 1) == 0) {
      // even
      n = n/2;
    } else {
      // odd
      n = 3*n + 1;
    }
  }
}

}
