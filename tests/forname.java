class forname {
    {
        System.out.println("<clinit>");
    }

    forname() {
        System.out.println("<init>");
    }

    public static void main(String[] argv) throws Throwable {
        Class.forName("forname").newInstance();
    }
}
