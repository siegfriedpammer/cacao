package org.cacaojvm.compiler2.test;

import org.junit.Test;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class ArrayTimesTwo extends Compiler2TestBase {
    @Test
    public void execute() throws Throwable{
		TimingResults tr = new TimingResults();

        int [] data = new int[1000000];
        for(int i = 0; i < data.length; i++){
            data[i] = i;
        }

        int[] dataBaseline = data.clone();
		int[] resultBaseline = (int[]) runBaseline("executeArrayTimesTwo", "([I)[I", tr.baseline, dataBaseline);

        int[] dataCompiler2 = data.clone();
		int[] resultCompiler2 = (int[]) runCompiler2("executeArrayTimesTwo", "([I)[I", tr.compiler2, dataCompiler2);
        
		assertArrayEquals(resultBaseline, resultCompiler2);
        
		tr.report();
    }

    static int[] executeArrayTimesTwo(int[] data){
        int[] result = new int[data.length];
        for (int i = 0; i < data.length; i++){
            result[i] = add(data[i], data[i]);
        }
        return result;
    }

    static int add(int a, int b){
        return a + b;
    }
}