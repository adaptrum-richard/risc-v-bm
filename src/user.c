static void delay(int loop)
{
    int j = 0;
    for(int i ; i < loop; i++){
        j = i % 3;
        j++;
    }
}

void user_process()
{
    int loop = 100000000;
    while(1){
        delay(loop);
    }
}