package com.antoniaklja.sample;

public class Main {


    public static void testMethod() {
        System.out.println("TEST METHOD!!");
    }


    public static void main(String[] args) throws Exception {
        Main.testMethod();
        throw new IllegalArgumentException("Test exception");
    }
}
