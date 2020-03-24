package org.cacaojvm.compiler2.test;

import org.junit.Test;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class ApplyToArray extends Compiler2TestBase {
    @Test
    public void execute() throws Throwable{
		TimingResults tr = new TimingResults();

        int [] data = new int[1000000];
        for(int i = 0; i < data.length; i++){
            data[i] = i;
        }

        int[] dataBaseline = data.clone();
		int[] resultBaseline = (int[]) runBaseline("executeApplyToArray", "([I)V", tr.baseline, dataBaseline);

        int[] dataCompiler2 = data.clone();
		int[] resultCompiler2 = (int[]) runCompiler2("executeApplyToArray", "([I)V", tr.compiler2, dataCompiler2);
        
		assertArrayEquals(resultBaseline, resultCompiler2);
        
		tr.report();
    }

    static void executeApplyToArray(int[] data){
        for (int i = 0; i < data.length; i++){
            data[i] = process(data[i]);
        }
    }

    static int process(int a){
        return getConstant() * 2 + a;
    }

    static int getConstant(){
        return 3;
    }
}