package org.cacaojvm.compiler2.test;

import org.junit.Test;
import java.util.Random;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class CreatePoints extends Compiler2TestBase {
    @Test
    public void execute() throws Throwable{
		TimingResults tr = new TimingResults();

        Random random = new Random();
        int [] x = new int[100000];
        int [] y = new int[100000];
        for(int i = 0; i < x.length; i++){
            x[i] = random.nextInt();
            y[i] = random.nextInt();
        }

        int[] xBaseline = x.clone();
        int[] yBaseline = y.clone();
		Point[] resultBaseline = (Point[]) runBaseline("createPoints", "([I[I)[Lorg/cacaojvm/compiler2/test/CreatePoints$Point;", tr.baseline, xBaseline, yBaseline);

        int[] xCompiler2 = x.clone();
        int[] yCompiler2 = y.clone();
		Point[] resultCompiler2 = (Point[]) runCompiler2("createPoints", "([I[I)[Lorg/cacaojvm/compiler2/test/CreatePoints$Point;", tr.compiler2, xCompiler2, yCompiler2);
        
		assertArrayEquals(resultBaseline, resultCompiler2);
        
		tr.report();
    }

    private static final class Point {
        private int x;
        private int y;

        public Point(int x, int y){
            this.x = x;
            this.y = y;
        }

        public int getX(){
            return x;
        }

        public int getY(){
            return y;
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

    static Point[] createPoints(int x[], int y[]) 
    { 
        int N = x.length;
        Point[] points = new Point[N];
        for(int i = 0; i < N; i++){
            points[i] = new Point(x[i], y[i]);
        }
        return points;
    }
}