static int[] categorizePoints(int[] x, int[] y) 
{ 
    int[] categories = new int[5];
    for(int i = 0; i < x.length; i++){
        Point p = new Point(x[i], y[i]);
        int index;
        if (p.getX() == 0 || p.getY() == 0){
            index = 0;
        } else if(p.getX() > 0){
            index = p.getY() > 0 ? 1 : 4;
        } else {
            index = p.getY() > 0 ? 2 : 3;
        }
        categories[index] += 1;
    }
    return categories;
}