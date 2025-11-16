#ifndef MY_DHT11_H
#define MY_DHT11_H

int dht11_init(void);
int dht11_read(int *temp, int *humidity);

#endif // MY_DHT11_H