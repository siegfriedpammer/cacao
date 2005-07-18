public class clinitexception {
    static class ThrowRuntimeException {
        static {
            if (true)
                throw new RuntimeException();
        }
    }

    public static void main(String[] argv) {
        try {
            new ThrowRuntimeException();
            System.out.println("FAILED");
        } catch (ExceptionInInitializerError e) {
            System.out.println("OK");
        }
    }
}
