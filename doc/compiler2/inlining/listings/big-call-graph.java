static int[] bigCallGraph(int[] x) 
{ 
    int N = x.length;
    int[] results = new int[N];
    for(int i = 0; i < N; i++){
        results[i] = process(x[i]);
    }
    return results;
}

static int process(int value){
    value = bigMethod(value);
    value = square(value);
    value = biggerThanTen(value) ? value : abs(value);
    int x = Math.min (0, 10);
    value = Math.max (x, value);
    for (int i = 0; i < 1000; i++){
        value = plusOne (value);
    }
    value = deeplyNestedFunc(value);
    return thousand() + value; 
}

static int bigMethod(int value){
    value *= 3;
    if(value > 0){
        value -= 1;
    }
    value++;
    int absVal = abs(value);
    value = square(absVal);
    return Math.max(value, 1);
}

static int mediumMethod(int value){
    for(int i = 0; i < 100; i++){
        value -= 1;
    }
    int absVal = abs(value);
    return value;
}

static boolean biggerThanTen(int value){
    return isRealPositive(value) && abs(value) > 10;
}

static int thousand(){
    return 1000;
}

static int abs (int value){
    return isRealPositive(value) ? value : -value;
}

static int square(int value){
    return value * value;
}

static int pluOne (int value){
    return value + 1;
}

static boolean isRealPositive(int value){
    return value > 0;
}

static int plusOne (int value){
    return value + 1;
}

static int deeplyNestedFunc (int value) {
    return deeplyNestedFunc1(value) + deeplyNestedFunc1(value);
}

static int deeplyNestedFunc1 (int value) {
    return deeplyNestedFunc2(value) + deeplyNestedFunc2(value);
}

static int deeplyNestedFunc2 (int value) {
    return deeplyNestedFunc3(value);
}

static int deeplyNestedFunc3 (int value) {
    return Math.abs(value);
}