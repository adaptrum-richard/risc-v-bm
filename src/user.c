static void delay()
{
    int j = 0;
    for(int i ; i < 100000000; i++){
        j = i % 3;
        j++;
    }
}

void user_process()
{
    while(1){
        delay();
    }
}