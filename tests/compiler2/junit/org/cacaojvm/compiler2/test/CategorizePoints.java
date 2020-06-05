package org.cacaojvm.compiler2.test;

import org.junit.Test;
import java.util.Random;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class CategorizePoints extends Compiler2TestBase {
    @Test
    public void execute() throws Throwable{
        TimingResults tr = new TimingResults();

        Random random = new Random();
        int [] x = new int[1000000];
        int [] y = new int[1000000];
        for(int i = 0; i < x.length; i++){
            x[i] = random.nextInt();
            y[i] = random.nextInt();
        }

        int[] xBaseline = x.clone();
        int[] yBaseline = y.clone();
		int[] resultBaseline = (int[]) runBaseline("categorizePoints", "([I[I)[I", tr.baseline, xBaseline, yBaseline);

        int[] xCompiler2 = x.clone();
        int[] yCompiler2 = y.clone();
		int[] resultCompiler2 = (int[]) runCompiler2("categorizePoints", "([I[I)[I", tr.compiler2, xCompiler2, yCompiler2);
        
		assertArrayEquals(resultBaseline, resultCompiler2);
        
		tr.report();
    }
      
    public final static class Point {
        private int x;
        private int y;

        public Point(int x, int y){
            this.x = x;
            this.y = y;
        }

        public final int getX(){
            return x;
        }

        public final int getY(){
            return y;
        }

        public final int getQuadrant(){
            if (getX() == 0 || getY() == 0){
                return 0;
            } else if(getX() > 0){
                return getY() > 0 ? 1 : 4;
            } else {
                return getY() > 0 ? 2 : 3;
            }
        }

        @Override
        public boolean equals(Object o){
            if (this == o) return true;
            if (o == null) return false;
            if (getClass() != o.getClass()) return false;
            Point point = (Point) o;
            return this.x == point.x && this.y == point.y;
        }
    }

    static int[] categorizePoints(int[] x, int[] y) 
    { 
        int[] categories = new int[5];
        for(int i = 0; i < x.length; i++){
            Point p = new Point(x[i], y[i]);
            categories[p.getQuadrant()] += 1;
        }
        return categories;
    }
}