static void applyToArray(int[] data){
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