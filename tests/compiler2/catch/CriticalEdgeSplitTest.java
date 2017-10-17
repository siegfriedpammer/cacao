
public class CriticalEdgeSplitTest {

	public static int noSplit() {
		int a = 5;
		int b = 10;
		return a + b;
	}

	public static int simpleIf() {
		int result = 10;
		if (result < 10) {
			result *= 2;
		}
		return result;
	}

	public static int noSplitIfElse() {
		int result = 10;
		if (result < 10) {
			result *= 2;
		} else {
			result *= 3;
		}
		return result;
	}

	// This code has 2 critical edges that are split
	public static int modifiedFact() {
		int res = 1;
		int n = 5;
		while (1 < n) {
			res *= n--;
			if (n == 2) break;
		}
		return res;
	}
}