package org.cacaojvm.compiler2.test;

import org.junit.Test;
import java.util.Random;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class BubbleSort extends Compiler2TestBase {
    @Test
    public void execute() throws Throwable{
		TimingResults tr = new TimingResults();

        Random random = new Random();
        int [] data = new int[10000];
        for(int i = 0; i < data.length; i++){
            data[i] = random.nextInt();
        }

        int[] dataBaseline = data.clone();
		runBaseline("bubbleSort", "([I)V", tr.baseline, dataBaseline);

        int[] dataCompiler2 = data.clone();
		runCompiler2("bubbleSort", "([I)V", tr.compiler2, dataCompiler2);
        
		assertArrayEquals(dataBaseline, dataCompiler2);
        
		tr.report();
    }

    static void bubbleSort(int arr[]) 
    { 
        int n = arr.length; 
        for (int i = 0; i < n-1; i++){ 
            for (int j = 0; j < n-i-1; j++) {
                if (arr[j] > arr[j+1]) 
                { 
                    int temp = arr[j]; 
                    arr[j] = arr[j+1]; 
                    arr[j+1] = temp; 
                } 
            }
        }
    }
}