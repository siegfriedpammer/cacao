import java.util.*;

public class testarguments {
    public static native void nsub();

    public static void main(String[] argv) {
        Random r = new Random();

        System.loadLibrary("testarguments");
        nsub();
    }

    public void jsub(int a0, float fa0) {
    }
}
