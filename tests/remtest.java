
// Zahlen summieren, Java Version

public class remtest {


	static public int rem(int i, int j) {
		return i % j;
		}


	static public void main(String [] arg) {

		int i;

		for (i = -1; i != 0; i--) {
			if ((i % 0x10001) != rem(i, 0x10001)) {
				System.out.println ("Error: " + i);
				}
			}

		System.out.println ("OK");
		}
	}
