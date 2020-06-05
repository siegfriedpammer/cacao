static Point[] createPoints(int x[], int y[]) 
{ 
    int N = x.length;
    Point[] points = new Point[N];
    for(int i = 0; i < N; i++){
        points[i] = new Point(x[i], y[i]);
    }
    return points;
}