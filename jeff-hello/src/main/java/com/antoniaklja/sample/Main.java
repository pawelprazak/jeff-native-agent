package com.antoniaklja.sample;

public class Main {

    public static void testMethod(byte test) {
        System.out.println("TEST METHOD " + test);
        Main.testMethod2(true);
    }

    public static void testMethod2(boolean test) {
        System.out.println("TEST METHOD " + test);
        String msg = "Test exception";
        Main.throwingMethod(msg);
    }

    public static void throwingMethod(String msg) {
        throw new IllegalArgumentException(msg);
    }

    public static void main(String[] args) throws Exception {
        byte test = 127;
        Main.testMethod(test);
    }
}
