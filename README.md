# Maxwell

## Description

NAT Hole Punching application.

# Author

Абишев Тимофей, Б05-232.

## Usage

Сначала необходимо запустить STUN сервер (сервер выключиться после обработки одной пары):

```sh
./Server server-ip server-port
```

После этого можно запускать клиентов (client-ip и client-port это адреса клиента в локальной сети):

```sh
./Client client-ip client-port server-ip server-port
```

В качестве проверки соединения клиенты обменяются 20 сообщениями.
