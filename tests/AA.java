public class AA
{
public void f() {
  int x = 1;
}
public static void main(String[] s) {
AA a;
DD d = new DD();
EE e = new EE();
int x = 1;

if (x==1)
  a=d;
else
  a=e;
a.f();
}
}
