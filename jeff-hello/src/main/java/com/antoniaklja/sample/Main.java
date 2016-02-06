package com.antoniaklja.sample;

public class Main {

    public static void testMethod() {
        System.out.println("TEST METHOD!!");
    }

    public static void throwingMethod() {
        String msg = "Test exception";
        throw new IllegalArgumentException(msg);
    }

    public static void main(String[] args) throws Exception {
        Main.testMethod();
        Main.throwingMethod();
    }
}
