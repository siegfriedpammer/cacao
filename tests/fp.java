public class fp {
	public static void main(String [] s) {

		float a=10,b=10;
		int i;
		
		for (i=0; i<1000; i++) {
			a*=b;
			p(a);
			}
		
		for (a=0; a<1; a+=0.2) {
			for (b=0; b<1; b+=0.2) {
				System.out.println ("-----------");
				p(a);
				p(b);
				p(a+b);
				p(a-b);
				p(a*b);
				p(a/b);
				}
			}
		}
		
		
	public static void p(double d) {
		System.out.println (d);
		}
	public static void p(float d) {
		System.out.println (d);
		}

	}
