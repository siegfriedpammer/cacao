import java.util.*;

public class sleep extends Thread {

    public sleep (String name) {
	super(name);
    }

    public void run() {
	Random random = new Random();

	try {
	    for (int i = 0; i < 10; ++i) {
		System.out.println(getName());
		sleep((long)(random.nextFloat() * 1000));
	    }
	} catch (Exception exc) {
	    System.out.println("Exception: " + exc);
	}
    }

    public static void main (String args[]) {
	sleep t1 = new sleep("a");
	sleep t2 = new sleep("b");

	t1.start();
	t2.start();
    }
}
