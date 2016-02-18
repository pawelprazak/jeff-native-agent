package com.antoniaklja.sample;

public class Main {

    public static void testMethod(String test1, int test2, long test3, byte test4, boolean test5) {
        System.out.printf("TEST METHOD %s %s %s %s %s%n", test1, test2, test3, test4, test5);
        Main.testMethod2();
    }

    public static void testMethod2() {
        String msg = "Test exception";
        Main.throwingMethod(msg);
    }

    public static void throwingMethod(String msg) {
        throw new IllegalArgumentException(msg);
    }

    public static void main(String[] args) throws Exception {
        String test1 = "test string";
        int test2 = 123;
        long test3 = 234;
        byte test4 = 127;
        boolean test5 = true;
        Main.testMethod(test1, test2, test3, test4, test5);
    }
}
