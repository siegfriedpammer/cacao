package org.cacaojvm.compiler2.test;

import org.junit.Test;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class Fibonacci extends Compiler2TestBase {
    @Test
    public void execute() throws Throwable{
        TimingResults tr = new TimingResults();
        
		int resultBaseline = (Integer) runBaseline("fibonacci", "(I)I", tr.baseline, 35);

		int resultCompiler2 = (Integer) runCompiler2("fibonacci", "(I)I", tr.compiler2, 35);
        System.out.println(resultCompiler2);
		assertEquals(resultBaseline, resultCompiler2);
        
		tr.report();
    }

    static int fibonacci(int n){
        return n <= 2 ? 1 : fibonacci(n - 2) + fibonacci(n - 1);
    }
}