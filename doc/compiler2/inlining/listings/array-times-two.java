static int[] arrayTimesTwo(int[] data){
    int[] result = new int[data.length];
    for (int i = 0; i < data.length; i++){
        result[i] = add(data[i], data[i]);
    }
    return result;
}

static int add(int a, int b){
    return a + b;
}